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

#include "trpc/server/service_adapter.h"

#include <algorithm>
#include <list>
#include <memory>
#include <optional>
#include <utility>

#include "trpc/codec/server_codec_factory.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_execution_context.h"
#include "trpc/filter/server_filter_manager.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/init_runtime.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_connection.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"
#include "trpc/runtime/threadmodel/separate/separate_thread_model.h"
#include "trpc/runtime/threadmodel/thread_model.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/server/server_context.h"
#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/core.h"
#include "trpc/transport/common/ssl/ssl.h"
#include "trpc/transport/common/ssl_helper.h"
#endif
#include "trpc/codec/http/http_protocol.h"
#include "trpc/transport/server/default/server_transport_impl.h"
#include "trpc/transport/server/fiber/fiber_server_transport_impl.h"
#include "trpc/transport/server/server_transport.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/object_pool/object_pool_ptr.h"
#include "trpc/util/time.h"

namespace trpc {

namespace {

FilterStatus RunServerFilters(FilterPoint filter_point, STransportMessage* msg) {
  return msg->context->GetFilterController().RunMessageServerFilters(filter_point, msg->context);
}

FilterStatus RunIoFilters(FilterPoint filter_point, const std::any& msg) noexcept {
  if (!msg.has_value()) {
    return FilterStatus::CONTINUE;
  }

  try {
    const auto& context = std::any_cast<const ServerContextPtr&>(msg);
    if (filter_point == FilterPoint::SERVER_POST_IO_SEND_MSG) {
      context->SetSendTimestampUs(trpc::time::GetMicroSeconds());
      auto& callback = context->GetSendMsgCallback();
      if (callback != nullptr) {
        callback();
      }
    }

    return context->GetFilterController().RunMessageServerFilters(filter_point, context);
  } catch (const std::exception& ex) {
    TRPC_FMT_ERROR("RunIoFilters throw exception: {}", ex.what());
    return FilterStatus::CONTINUE;
  }
}

}  // namespace

ServiceAdapter::ServiceAdapter(ServiceAdapterOption&& option) : option_(std::move(option)) {
  TRPC_ASSERT(Initialize());
}

bool ServiceAdapter::Initialize() {
  server_codec_ = ServerCodecFactory::GetInstance()->Get(option_.protocol);
  if (!server_codec_) {
    TRPC_FMT_ERROR("service name: {}, get service codec {} failed.", option_.service_name, option_.protocol);
    return false;
  }

  TRPC_FMT_DEBUG("service name: {}, thread_model instance name: {}", option_.service_name,
                 option_.threadmodel_instance_name);

  thread_model_ = [this]() {
    ThreadModel* thread_model;
    if (!option_.threadmodel_instance_name.empty()) {
      thread_model = ThreadModelManager::GetInstance()->Get(option_.threadmodel_instance_name);
    } else {
      TRPC_FMT_DEBUG("service name: {}, thread_model instance name is empty.", option_.service_name);
      if (runtime::IsInFiberRuntime()) {
        thread_model = fiber::GetFiberThreadModel();
      } else {
        thread_model = merge::RandomGetMergeThreadModel();
        if (thread_model == nullptr) {
          thread_model = separate::RandomGetSeparateThreadModel();
        }
      }
    }
    return thread_model;
  }();

  if (!thread_model_) {
    TRPC_FMT_ERROR("service name: {}, get thread_model instance name: {} failed.",
                   option_.service_name, option_.threadmodel_instance_name);
    return false;
  }

  return true;
}

bool ServiceAdapter::Listen() {
  if (transport_ && !is_listened_) {
    if (transport_->Listen()) {
      is_listened_ = true;
      return true;
    }
  } else if (is_listened_) {
    return true;
  }

  TRPC_FMT_ERROR("listen failed, maybe transport_ listen fail or transport_ is nullptr");
  return false;
}

bool ServiceAdapter::StopListen(bool clean_conn) {
  if (!transport_) {
    TRPC_FMT_ERROR("ServiceAdapter::StopListen failed, transport_ is nullptr.");
    return false;
  }

  if (!is_listened_) {
    return true;
  }

  transport_->StopListen(clean_conn);

  auto_start_ = false;
  is_listened_ = false;

  return true;
}

void ServiceAdapter::Stop() {
  if (transport_) {
    transport_->Stop();
    auto_start_ = false;
    is_listened_ = false;
  }
}

void ServiceAdapter::Destroy() {
  if (transport_) {
    transport_->Destroy();
  }
}

void ServiceAdapter::SetService(const ServicePtr& service) {
  for (auto& filter_name : option_.service_filters) {
    auto filter = ServerFilterManager::GetInstance()->GetMessageServerFilter(filter_name);
    if (filter) {
      AddServiceFilter(service->GetFilterController(), filter);
    }
  }

  services_[service->GetName()] = service;
  if (is_shared_ && transport_) {
    return;
  }

  service_ = service;
  transport_ = CreateServerTransport();
  TRPC_ASSERT(transport_);

  trpc::BindInfo bind_info;
  FillBindInfo(bind_info);

  transport_->Bind(bind_info);

  service_->SetServerTransport(transport_.get());
}

std::unique_ptr<ServerTransport> ServiceAdapter::CreateServerTransport() {
  // prioritize the method provided by the service to create the server transport
  std::unique_ptr<ServerTransport> transport = service_->CreateTransport(option_, thread_model_);
  if (!transport) {
    TRPC_ASSERT(thread_model_ != nullptr);
    if (thread_model_->Type() == kFiber) {
      transport = std::make_unique<FiberServerTransportImpl>();
    } else if (thread_model_->Type() == kSeparate) {
      SeparateThreadModel* threadmodel = static_cast<SeparateThreadModel*>(thread_model_);
      transport = std::make_unique<ServerTransportImpl>(threadmodel->GetReactors());
    } else if (thread_model_->Type() == kMerge) {
      MergeThreadModel* threadmodel = static_cast<MergeThreadModel*>(thread_model_);
      transport = std::make_unique<ServerTransportImpl>(threadmodel->GetReactors());
    } else {
      TRPC_LOG_ERROR("service name:" << option_.service_name << ", thread_model_type: " << thread_model_->Type()
                                     << ", invalid.");
      return nullptr;
    }
  }

  return transport;
}

STransportReqMsg* ServiceAdapter::CreateSTransportReqMsg(const ConnectionPtr& conn, uint64_t recv_timestamp_us,
                                                         std::any&& msg) {
  ServerContextPtr context = MakeRefCounted<ServerContext>();

  context->SetRecvTimestampUs(recv_timestamp_us);
  context->SetConnectionId(conn->GetConnId());
  context->SetFd(conn->GetFd());

  ServerContext::NetType network_type = ServerContext::NetType::kTcp;
  if (conn->GetConnType() == ConnectionType::kUdp) {
    network_type = ServerContext::NetType::kUdp;
  } else if (conn->GetConnType() == ConnectionType::kOther) {
    network_type = ServerContext::NetType::kOther;
  }
  context->SetNetType(network_type);
  context->SetPort(conn->GetPeerPort());
  context->SetIp(conn->GetPeerIp());
  context->SetServerCodec(server_codec_.get());
  context->SetRequestMsg(server_codec_->CreateRequestObject());
  context->SetResponseMsg(server_codec_->CreateResponseObject());

  bool ret = server_codec_->ZeroCopyDecode(context, std::move(msg), context->GetRequestMsg());
  if (!ret) {
    TRPC_FMT_ERROR_EVERY_SECOND("request header decode failed, ip: {}, port: {}", context->GetIp(), context->GetPort());
    auto& status = context->GetStatus();
    status.SetFrameworkRetCode(GetDefaultServerRetCode(codec::ServerRetCode::DECODE_ERROR));
    status.SetErrorMessage("request header decode failed");
  }

  Service* service = ChooseService(context->GetRequestMsg().get());
  service->FillServerContext(context);
  context->SetService(service);
  context->SetFilterController(&(service->GetFilterController()));

  auto* req_msg = trpc::object_pool::New<STransportReqMsg>();
  req_msg->context = std::move(context);

  return req_msg;
}

Service* ServiceAdapter::ChooseService(Protocol* protocol) const {
  if (TRPC_LIKELY(!is_shared_)) {
    return service_.get();
  }

  auto it = services_.find(protocol->GetCalleeName());
  if (TRPC_UNLIKELY(it == services_.end())) {
    TRPC_FMT_ERROR("maybe service name {} not register in this adapter or decode failed", protocol->GetCalleeName());

    return service_.get();
  }

  return it->second.get();
}

bool ServiceAdapter::HandleMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) {
  if (TRPC_UNLIKELY(!service_)) {
    TRPC_FMT_ERROR("handle message failed, service not existed");
    return false;
  }

  uint64_t recv_timestamp_us = trpc::time::GetMicroSeconds();
  for (std::any& it : msg) {
    auto* req_msg = CreateSTransportReqMsg(conn, recv_timestamp_us, std::move(it));

    // if the decoding fails or the filter intercepts, set succ flag is false
    bool succ = req_msg->context->GetStatus().OK();
    if (RunServerFilters(FilterPoint::SERVER_PRE_SCHED_RECV_MSG, req_msg) == FilterStatus::REJECT) {
      succ = false;
    }

    SetLocalServerContext(req_msg->context);

    MsgTaskHandler msg_handler = [this, req_msg]() mutable {
      auto& context = req_msg->context;

      context->SetBeginTimestampUs(trpc::time::GetMicroSeconds());

      RunServerFilters(FilterPoint::SERVER_POST_SCHED_RECV_MSG, req_msg);

      STransportRspMsg* send = nullptr;
      Service* service = context->GetService();
      service->HandleTransportMessage(req_msg, &send);

      context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

      trpc::object_pool::Delete(req_msg);

      if (send) {
        this->transport_->SendMsg(send);
        SetLocalServerContext(nullptr);
      }
    };

    if (TRPC_UNLIKELY(!succ)) {
      msg_handler();
      continue;
    }

    MsgTask* task = object_pool::New<MsgTask>();
    task->handler = std::move(msg_handler);
    task->group_id = thread_model_->GroupId();

    Service* service = req_msg->context->GetService();
    HandleRequestDispatcherFunction& dispatcher = service->GetHandleRequestDispatcherFunction();
    if (dispatcher) {
      task->dst_thread_key = dispatcher(req_msg);
    }

    if (!thread_model_->SubmitHandleTask(task)) {
      auto& context = req_msg->context;
      Status& status = context->GetStatus();
      if (status.OK()) {
        status.SetFrameworkRetCode(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR);
      }

      // When an error occurs, logging is used to record it.
      // However, if there is an overload, the logs may flood the system.
      // To prevent this, we will limit the logging to once per second.
      TRPC_FMT_ERROR_EVERY_SECOND("SubmitHandleTask failed, service name: {}, thread_model: {}",
                                  context->GetService()->GetName(), thread_model_->Type());
      RunServerFilters(FilterPoint::SERVER_POST_SCHED_RECV_MSG, req_msg);

      trpc::object_pool::Delete(req_msg);
    }
  }

  return true;
}

bool ServiceAdapter::HandleFiberMessage(const ConnectionPtr& conn, std::deque<std::any>& msg) {
  if (TRPC_UNLIKELY(!service_)) {
    TRPC_FMT_ERROR("handle message, but service not exist");
    return false;
  }

  uint64_t recv_timestamp_us = trpc::time::GetMicroSeconds();
  for (std::any& it : msg) {
    conn->Ref();

    auto* req_msg = CreateSTransportReqMsg(conn, recv_timestamp_us, std::move(it));

    // if the decoding fails or the filter intercepts, set succ flag is false
    bool succ = req_msg->context->GetStatus().OK();
    if (RunServerFilters(FilterPoint::SERVER_PRE_SCHED_RECV_MSG, req_msg) == FilterStatus::REJECT) {
      succ = false;
    }

    MsgTaskHandler msg_handler = [this, conn, req_msg]() mutable {
      auto& context = req_msg->context;
      context->SetBeginTimestampUs(trpc::time::GetMicroSeconds());

      RunServerFilters(FilterPoint::SERVER_POST_SCHED_RECV_MSG, req_msg);
      SetLocalServerContext(req_msg->context);

      STransportRspMsg* send = nullptr;
      Service* service = context->GetService();
      if (!service->GetNeedFiberExecutionContext()) {
        service->HandleTransportMessage(req_msg, &send);
      } else {
        auto exec_ctx = FiberExecutionContext::Create();
        exec_ctx->Execute([service, req_msg, &send] { service->HandleTransportMessage(req_msg, &send); });
      }

      context->SetEndTimestampUs(trpc::time::GetMicroSeconds());

      trpc::object_pool::Delete(req_msg);

      if (send) {
        send->context->SetReserved(static_cast<void*>(conn.Get()));
        this->transport_->SendMsg(send);
        SetLocalServerContext(nullptr);
      }

      conn->Deref();
    };

    if (TRPC_UNLIKELY(!succ)) {
      msg_handler();
      continue;
    }

    MsgTask* task = object_pool::New<MsgTask>();
    task->handler = std::move(msg_handler);
    task->group_id = thread_model_->GroupId();

    Service* service = req_msg->context->GetService();
    HandleRequestDispatcherFunction& dispatcher = service->GetHandleRequestDispatcherFunction();
    if (dispatcher) {
      task->dst_thread_key = dispatcher(req_msg);
    }

    bool result = thread_model_->SubmitHandleTask(task);
    if (!result) {
      auto& context = req_msg->context;
      Status& status = context->GetStatus();

      if (status.OK()) {
        status.SetFrameworkRetCode(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR);
      }

      RunServerFilters(FilterPoint::SERVER_POST_SCHED_RECV_MSG, req_msg);

      conn->Deref();

      trpc::object_pool::Delete(req_msg);
    }
  }

  return true;
}

void ServiceAdapter::SetSSLConfigToBindInfo(trpc::BindInfo& bind_info) {
  // Set SSL/TLS context and options for server.
#ifdef TRPC_BUILD_INCLUDE_SSL
  // Init Ssl options and context.
  // Note: ssl_ctx also indicates ssl is enable or not, enable ssl if ssl_ctx is not nullptr when connection was
  // created.
  if (option_.ssl_config.enable) {
    // Init SSL options
    ssl::ServerSslOptions ssl_options;
    TRPC_ASSERT(ssl::InitServerSslOptions(option_.ssl_config, &ssl_options));

    // Init SSL context
    ssl::SslContextPtr ssl_ctx = MakeRefCounted<ssl::SslContext>();
    TRPC_ASSERT(ssl_ctx->Init(ssl_options));

    bind_info.ssl_options = std::move(ssl_options);
    bind_info.ssl_ctx = std::move(ssl_ctx);
  } else {
    bind_info.ssl_ctx = nullptr;
  }
#endif
}

void ServiceAdapter::FillBindInfo(trpc::BindInfo& bind_info) {
  bind_info.socket_type = option_.socket_type;
  bind_info.ip = option_.ip;
  bind_info.is_ipv6 = option_.is_ipv6;
  bind_info.port = option_.port;
  bind_info.network = option_.network;
  bind_info.unix_path = option_.unix_path;
  bind_info.protocol = option_.protocol;
  bind_info.idle_time = option_.idle_time;
  bind_info.max_conn_num = option_.max_conn_num;
  bind_info.max_packet_size = option_.max_packet_size;
  bind_info.recv_buffer_size = option_.recv_buffer_size;
  bind_info.send_queue_capacity = option_.send_queue_capacity;
  bind_info.send_queue_timeout = option_.send_queue_timeout;
  bind_info.accept_thread_num = option_.accept_thread_num;
  bind_info.accept_function = service_->GetAcceptConnectionFunction();
  bind_info.dispatch_accept_function = service_->GetDispatchAcceptConnectionFunction();
  bind_info.conn_establish_function = service_->GetConnectionEstablishFunction();
  bind_info.conn_close_function = service_->GetConnectionCloseFunction();
  bind_info.checker_function = service_->GetProtocalCheckerFunction();
  bind_info.msg_handle_function = service_->GetMessageHandleFunction();
  bind_info.custom_set_socket_opt_function = service_->GetCustomSetSocketOptFunction();
  bind_info.custom_set_accept_socket_opt_function = service_->GetCustomSetAcceptSocketOptFunction();

  // Compatibility with special cases, enabling the use of trpc_over_http even when the protocol is configured as http.
  if (bind_info.protocol == kHttpCodecName && !service_->GetRpcServiceMethod().empty()) {
    TRPC_LOG_INFO("Compatibility with trpc_over_http even when the protocol is configured as http");
    server_codec_ = ServerCodecFactory::GetInstance()->Get(kTrpcOverHttpCodecName);
    TRPC_ASSERT(server_codec_ && "codec of trpc_over_http not found");
  }

  if (!bind_info.checker_function) {
    bind_info.checker_function = [codec = server_codec_.get()](const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                                               std::deque<std::any>& out) {
      return codec->ZeroCopyCheck(conn, in, out);
    };
  }

  if (!bind_info.msg_handle_function) {
    if (thread_model_->Type() == kFiber) {
      TRPC_FMT_DEBUG("service {} fiber HandleFiberMessage", option_.service_name);
      bind_info.msg_handle_function = [this](const ConnectionPtr& conn, std::deque<std::any>& msg) {
        return HandleFiberMessage(conn, msg);
      };
    } else {
      TRPC_FMT_DEBUG("service {} default HandleMessage", option_.service_name);
      bind_info.msg_handle_function = [this](const ConnectionPtr& conn, std::deque<std::any>& msg) {
        return HandleMessage(conn, msg);
      };
    }
  }

  bind_info.run_server_filters_function = [this](FilterPoint filter_point, STransportMessage* msg) {
    return RunServerFilters(filter_point, msg);
  };

  bind_info.run_io_filters_function = [this](FilterPoint filter_point, const std::any& msg) {
    return RunIoFilters(filter_point, msg);
  };

  SetSSLConfigToBindInfo(bind_info);

  bind_info.has_stream_rpc = service_->AnyOfStreamingRpc();
  if (bind_info.has_stream_rpc) {
    bind_info.run_server_filters_function = [](FilterPoint filter_point, STransportMessage* msg) {
      return FilterStatus::CONTINUE;
    };
    bind_info.handle_streaming_rpc_function = [this](STransportReqMsg* recv) {
      recv->context->SetFilterController(&(this->service_->GetFilterController()));
      this->service_->HandleTransportStreamMessage(recv);
    };
  }

  bind_info.check_stream_rpc_function = [this](const std::string& func_name) {
    const auto& rpc_service_methods = service_->GetRpcServiceMethod();
    auto it = rpc_service_methods.find(func_name);
    if (it == rpc_service_methods.end()) {
      return false;
    }
    return it->second->GetMethodType() != MethodType::UNARY;
  };
}

void ServiceAdapter::AddServiceFilter(ServerFilterController& filter_controller, const MessageServerFilterPtr& filter) {
  auto& filter_configs = option_.service_filter_configs;
  auto iter = filter_configs.find(filter->Name());
  std::any param;
  if (iter != filter_configs.end()) {
    param = iter->second;
  }
  auto new_filter = filter->Create(param);
  if (new_filter != nullptr) {
    filter_controller.AddMessageServerFilter(new_filter);
  } else {
    filter_controller.AddMessageServerFilter(filter);
  }
}

}  // namespace trpc
