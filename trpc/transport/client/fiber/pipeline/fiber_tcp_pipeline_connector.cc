//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "trpc/transport/client/fiber/pipeline/fiber_tcp_pipeline_connector.h"

#include <limits>
#include <mutex>
#include <utility>
#include <vector>

#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_timer.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/iomodel/reactor/common/io_handler.h"
#include "trpc/runtime/iomodel/reactor/common/socket.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_tcp_connection.h"
#include "trpc/transport/client/common/client_io_handler_factory.h"
#include "trpc/transport/client/fiber/common/fiber_client_connection_handler.h"
#include "trpc/transport/client/fiber/common/fiber_client_connection_handler_factory.h"
#include "trpc/transport/client/fiber/pipeline/fiber_tcp_pipeline_connector_group.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/likely.h"
#include "trpc/util/time.h"

namespace trpc {

FiberTcpPipelineConnector::FiberTcpPipelineConnector(const Options& options)
    : options_(options) {
  send_request_id_queue_.Init(INT16_MAX);
  // memory allocate: size * 64, if memory usage high, reduce fiber_pipeline_connector_queue_size
  send_request_id_queue_.Init(options_.trans_info->fiber_pipeline_connector_queue_size);
}

FiberTcpPipelineConnector::~FiberTcpPipelineConnector() {
  ClearReqMsg();

  if (connection_ != nullptr) {
    connection_.Reset();
  }
}

bool FiberTcpPipelineConnector::Init() {
  call_map_ = MakeRefCounted<CallMap>();
  return CreateFiberTcpConnection(options_.conn_id);
}

void FiberTcpPipelineConnector::Stop() {
  SetHealthy(false);
  if (cleanup_.exchange(true)) {
    ClearReqMsg();
    return;
  }

  ClearResource();
}

void FiberTcpPipelineConnector::Destroy() {
  if (connection_ != nullptr) {
    connection_.Reset();
  }
}

bool FiberTcpPipelineConnector::CreateFiberTcpConnection(uint64_t conn_id) {
  auto socket = Socket::CreateTcpSocket(options_.peer_addr->IsIpv6());
  if (!socket.IsValid()) {
    TRPC_LOG_ERROR("create tcp socket failed on peer addr:" << options_.peer_addr->ToString());
    return false;
  }

  socket.SetTcpNoDelay();
  socket.SetCloseWaitDefault();
  socket.SetBlock(false);
  socket.SetKeepAlive();

  if (options_.trans_info->set_socket_opt_function) {
    options_.trans_info->set_socket_opt_function(socket);
  }

  Reactor* reactor = trpc::fiber::GetReactor(trpc::fiber::GetCurrentSchedulingGroupIndex(), -2);
  TRPC_ASSERT(reactor != nullptr && "reactor is null");

  auto conn = MakeRefCounted<FiberTcpConnection>(reactor, socket);
  std::unique_ptr<IoHandler> io_handler =
      ClientIoHandlerFactory::GetInstance()->Create(options_.trans_info->protocol, conn.Get(), options_.trans_info);

  conn->SetMaxPacketSize(options_.trans_info->max_packet_size);
  conn->SetRecvBufferSize(options_.trans_info->recv_buffer_size);
  conn->SetSendQueueCapacity(options_.trans_info->send_queue_capacity);
  conn->SetSendQueueTimeout(options_.trans_info->send_queue_timeout);
  conn->SetConnId(options_.conn_id);
  conn->SetClient();
  conn->SetPeerIp(options_.peer_addr->Ip());
  conn->SetPeerPort(options_.peer_addr->Port());
  conn->SetPeerIpType(options_.peer_addr->Type());
  conn->SetIoHandler(std::move(io_handler));

  this->Ref();
  auto conn_handler = FiberClientConnectionHandlerFactory::GetInstance()->Create(
      options_.trans_info->protocol, conn.Get(), options_.trans_info);
  conn_handler->SetCleanFunc([this](Connection* conn) {
    this->ConnectionCleanFunction(conn);
  });
  conn_handler->SetMsgHandleFunc([this](const ConnectionPtr& conn, std::deque<std::any>& data) {
    return this->MessageHandleFunction(conn, data);
  });
  conn_handler->Init();

  conn->SetConnectionHandler(std::move(conn_handler));
  conn->SetSupportPipeline();

  auto* fiber_conn_handler = static_cast<FiberClientConnectionHandler*>(conn->GetConnectionHandler());
  fiber_conn_handler->SetPipelineSendNotifyFunc(
      [this](const IoMessage& io_msg) { this->NotifyPipelineSendMsg(io_msg.seq_id); });

  connection_ = std::move(conn);

  if (connection_->DoConnect()) {
    SetHealthy(true);
    connection_->StartHandshaking();
    return true;
  }

  SetHealthy(false);
  connection_.Reset();

  this->Deref();

  TRPC_LOG_ERROR("DoConnect fail, remote addr:" << options_.peer_addr->ToString());

  return false;
}

void FiberTcpPipelineConnector::ConnectionCleanFunction(Connection* conn) {
  SetHealthy(false);

  if (cleanup_.exchange(true)) {
    return;
  }

  TRPC_LOG_WARN("ConnectionCleanFunction");

  RefPtr connector(ref_ptr, this);

  bool start_fiber = StartFiberDetached([this, connector]() mutable {
    ClearResource();
  });

  TRPC_ASSERT(start_fiber && "StartFiberDetached failed when ConnectionCleanFunction");
}

bool FiberTcpPipelineConnector::IsConnIdleTimeout() const {
  if (connection_ != nullptr) {
    uint64_t now_ms = trpc::time::GetMilliSeconds();
    if (options_.trans_info->connection_idle_timeout == 0 ||
        (connection_->GetConnActiveTime() + options_.trans_info->connection_idle_timeout) > now_ms) {
      return false;
    }
    return true;
  }

  return false;
}

void FiberTcpPipelineConnector::CloseConnection() {
  SetHealthy(false);

  if (cleanup_.exchange(true)) {
    return;
  }

  TRPC_LOG_WARN("CloseConnection");

  bool start_fiber = StartFiberDetached([this, ref = RefPtr(ref_ptr, this)]() mutable {
    ClearResource();
  });

  TRPC_ASSERT(start_fiber && "StartFiberDetached failed when CloseConnection");
}

void FiberTcpPipelineConnector::ClearReqMsg() {
  uint32_t req_id = 0;
  while (send_request_id_queue_.Dequeue(req_id) == LockFreeQueue<uint32_t>::RT_OK) {
    TRPC_LOG_WARN("ClearReqMsg req_id:" << req_id);
  }

  std::vector<uint64_t> ids;

  call_map_->ForEach([&ids](auto&& request_id, auto&& ctx) { ids.push_back(request_id); });
  for (auto request_id : ids) {
    TRPC_LOG_WARN("ClearReqMsg request_id:" << request_id);
    DispatchException(request_id, TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, "connector destroy");
  }
}

void FiberTcpPipelineConnector::ClearResource() {
  ClearReqMsg();

  TRPC_ASSERT(UnsafeRefCount() && "Some exception happened, and this FiberConnector released");

  connection_->GetConnectionHandler()->Stop();
  connection_->Stop();

  connection_->GetConnectionHandler()->Join();
  connection_->Join();

  this->Deref();
}

void FiberTcpPipelineConnector::SendReqMsg(CTransportReqMsg* req_msg) {
  Send(req_msg, false);
}

void FiberTcpPipelineConnector::SendOnly(CTransportReqMsg* req_msg) {
  Send(req_msg, true);
}

void FiberTcpPipelineConnector::Send(CTransportReqMsg* req_msg, bool is_oneway) {
  TRPC_ASSERT(connection_.Get() != nullptr);

  if (options_.trans_info->run_client_filters_function) {
    options_.trans_info->run_client_filters_function(FilterPoint::CLIENT_PRE_SCHED_SEND_MSG, req_msg);
    options_.trans_info->run_client_filters_function(FilterPoint::CLIENT_POST_SCHED_SEND_MSG, req_msg);
  }

  IoMessage message;
  message.seq_id = req_msg->context->GetRequestId();
  message.context_ext = req_msg->context->GetCurrentContextExt();
  message.is_oneway = is_oneway;
  message.buffer = std::move(req_msg->send_data);
  message.msg = req_msg->context;

  // Sending Non-Streaming Unary Requests.
  if (req_msg->context->GetStreamId() == 0) {
    connection_->Send(std::move(message));
  } else {
    // Sending Streaming Unary Responses, which may require additional encoding such as gRPC/HTTP2.
    auto fiber_conn_handler_ = static_cast<FiberClientConnectionHandler*>(connection_->GetConnectionHandler());
    if (TRPC_LIKELY(fiber_conn_handler_->EncodeStreamMessage(&message))) {
      if (fiber_conn_handler_->NeedSendStreamMessage()) {
        connection_->Send(std::move(message));
      }
    }
  }
}

bool FiberTcpPipelineConnector::MessageHandleFunction(const ConnectionPtr& conn, std::deque<std::any>& rsp_list) {
  bool conn_reusable = true;
  for (auto&& rsp_buf : rsp_list) {
    ProtocolPtr rsp_protocol;
    bool ret = options_.trans_info->rsp_decode_function(std::move(rsp_buf), rsp_protocol);
    if (TRPC_LIKELY(ret)) {
      TRPC_ASSERT(rsp_protocol);
      conn_reusable &= rsp_protocol->IsConnectionReusable();

      auto id = GetMatchedRequestId();
      if (TRPC_UNLIKELY(id == std::nullopt)) {
        conn_reusable = false;
        SetHealthy(false);
        TRPC_LOG_WARN("can not find request id, maybe cleared");
        break;
      }

      auto ctx = call_map_->TryReclaimContext(*id);
      if (TRPC_UNLIKELY(ctx.Get() == nullptr)) {
        // The request corresponding to the response cannot be found,
        // and the request may have timed out, so it will not be processed
        TRPC_LOG_WARN("can not find request, id: " << *id << ", maybe timeout");
        continue;
      }

      if (ctx->backup_request_retry_info == nullptr) {
        ctx->rsp_msg->msg = std::move(rsp_protocol);

        if (options_.trans_info->run_client_filters_function) {
          options_.trans_info->run_client_filters_function(FilterPoint::CLIENT_PRE_SCHED_RECV_MSG, ctx->req_msg);
          options_.trans_info->run_client_filters_function(FilterPoint::CLIENT_POST_SCHED_RECV_MSG, ctx->req_msg);
        }
      } else {
        if (!ctx->backup_request_retry_info->retry->IsReplyReady()) {
          ctx->rsp_msg->msg = std::move(rsp_protocol);

          if (options_.trans_info->run_client_filters_function) {
            options_.trans_info->run_client_filters_function(FilterPoint::CLIENT_PRE_SCHED_RECV_MSG, ctx->req_msg);
            options_.trans_info->run_client_filters_function(FilterPoint::CLIENT_POST_SCHED_RECV_MSG, ctx->req_msg);
          }
        } else {
          TRPC_LOG_WARN("Get response late when retry, if too many this log, consider a bigger retry delay");
        }
      }

      DispatchResponse(std::move(ctx));
    } else {
      conn_reusable = false;
      break;
    }
  }
  return conn_reusable;
}

bool FiberTcpPipelineConnector::SaveCallContext(CTransportReqMsg* req_msg, CTransportRspMsg* rsp_msg,
                                                OnCompletionFunction&& cb) {
  if (!IsHealthy()) {
    cb(TrpcRetCode::TRPC_CLIENT_NETWORK_ERR, "network error.");
    return false;
  }

  uint32_t request_id = req_msg->context->GetRequestId();
  auto&& [ctx, lock] = call_map_->AllocateContext(request_id);
  (void)lock;
  ctx->req_msg = req_msg;
  ctx->rsp_msg = rsp_msg;
  ctx->request_id = request_id;
  ctx->backup_request_retry_info = req_msg->context->GetBackupRequestRetryInfo();
  ctx->on_completion_function = std::move(cb);

  ctx->timeout_timer = CreateTimer(request_id, req_msg->context->GetTimeout());
  EnableFiberTimer(ctx->timeout_timer);

  return true;
}

uint64_t FiberTcpPipelineConnector::CreateTimer(uint32_t request_id, uint32_t timeout) {
  auto timeout_cb = [this, ref = RefPtr(ref_ptr, this), request_id](auto) {
    OnTimeout(request_id);
  };
  return CreateFiberTimer(ReadSteadyClock() + std::chrono::milliseconds(timeout),
                          std::move(timeout_cb));
}

void FiberTcpPipelineConnector::OnTimeout(uint32_t request_id) {
  TRPC_ASSERT(UnsafeRefCount() && "Some exception happened, and this FiberTcpPipelineConnector released");

  DispatchException(request_id, TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR, "request timeout");
}

void FiberTcpPipelineConnector::DispatchException(uint32_t request_id, int ret, std::string&& err_msg) {
  auto ctx = call_map_->TryReclaimContext(request_id);
  if (ctx.Get() == nullptr) {
    return;
  }

  {
    // Make sure context initialize ok
    std::scoped_lock _(ctx->lock);
  }

  if (auto t = std::exchange(ctx->timeout_timer, 0)) {
    KillFiberTimer(t);
  }

  TRPC_ASSERT(UnsafeRefCount() > 0);
  err_msg += ", request_id: ";
  err_msg += std::to_string(request_id);
  err_msg += ", remote: ";
  err_msg += options_.peer_addr->ToString();

  ctx->on_completion_function(ret, std::move(err_msg));
}

void FiberTcpPipelineConnector::DispatchException(object_pool::LwUniquePtr<CallContext> ctx, int ret,
    std::string&& err_msg) {
  {
    // Make sure context initialize ok
    std::scoped_lock _(ctx->lock);
  }

  if (auto t = std::exchange(ctx->timeout_timer, 0)) {
    KillFiberTimer(t);
  }
  ctx->on_completion_function(ret, std::move(err_msg));
}

void FiberTcpPipelineConnector::DispatchResponse(object_pool::LwUniquePtr<CallContext> ctx) {
  if (auto t = std::exchange(ctx->timeout_timer, 0)) {
    KillFiberTimer(t);
  }

  ctx->on_completion_function(0, "");
}

void FiberTcpPipelineConnector::NotifyPipelineSendMsg(uint32_t request_id) {
  TRPC_LOG_WARN("NotifyPipelineSendMsg remote:" << options_.peer_addr->ToString() << ", request_id:" << request_id <<
                 ", queue size:" << send_request_id_queue_.Size());
  TRPC_ASSERT(send_request_id_queue_.Enqueue(request_id) == LockFreeQueue<uint32_t>::RT_OK);
}

std::optional<uint32_t> FiberTcpPipelineConnector::GetMatchedRequestId() {
  uint32_t req_id = 0;
  if (TRPC_LIKELY(send_request_id_queue_.Dequeue(req_id) == LockFreeQueue<uint32_t>::RT_OK)) {
    TRPC_LOG_WARN("GetMatchedRequestId remote:" << options_.peer_addr->ToString() << ", req_id:" << req_id <<
                   ", queue size:" << send_request_id_queue_.Size());
    return req_id;
  }

  return std::nullopt;
}

}  // namespace trpc
