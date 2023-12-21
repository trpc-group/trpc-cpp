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

#include "trpc/client/service_proxy.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <memory>
#include <utility>

#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/common/config/default_value.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/naming/selector_factory.h"
#include "trpc/naming/trpc_naming.h"
#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/runtime/fiber_runtime.h"
#include "trpc/runtime/init_runtime.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/runtime/threadmodel/thread_model_manager.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/stream/stream_handler.h"
#include "trpc/transport/client/future/future_transport.h"
#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/core.h"
#include "trpc/transport/common/ssl/ssl.h"
#include "trpc/transport/common/ssl_helper.h"
#endif
#include "trpc/transport/client/fiber/fiber_transport.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/domain_util.h"
#include "trpc/util/net_util.h"
#include "trpc/util/string/string_util.h"
#include "trpc/util/time.h"
#include "trpc/util/unique_id.h"

namespace trpc {

static UniqueId unique_id;

Status ServiceProxy::UnaryInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) {
  TRPC_FMT_DEBUG("UnaryInvoke msg request_id: {}", context->GetRequestId());

  // full-link timeout check, it will immediately return a failure if there is no timeout left
  if (CheckTimeout(context)) {
    return context->GetStatus();
  }

  // Run filters before client sends the RPC request message
  int filter_ret = RunFilters(FilterPoint::CLIENT_PRE_SEND_MSG, context);
  if (filter_ret == 0) {
    UnaryTransportInvoke(context, req, rsp);

    ProxyStatistics(context);
  }

  // Run filters after client receives the RPC response message
  RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);

  return context->GetStatus();
}

Future<ProtocolPtr> ServiceProxy::AsyncUnaryInvoke(const ClientContextPtr& context, const ProtocolPtr& req) {
  TRPC_FMT_DEBUG("UnaryInvoke msg request_id: {}", context->GetRequestId());

  if (CheckTimeout(context)) {
    const Status& result = context->GetStatus();
    return MakeExceptionFuture<ProtocolPtr>(
        CommonException(result.ErrorMessage().c_str(), result.GetFrameworkRetCode()));
  }

  int filter_ret = RunFilters(FilterPoint::CLIENT_PRE_SEND_MSG, context);
  if (filter_ret == 0) {
    return AsyncUnaryTransportInvoke(context, req);
  } else {
    RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);
    return MakeExceptionFuture<ProtocolPtr>(CommonException(context->GetStatus().ErrorMessage().c_str()));
  }
}

bool ServiceProxy::CheckTimeout(const ClientContextPtr& context) {
  if (context->GetTimeout() == 0) {
    std::string error("service name:");
    error += GetServiceName();

    Status result;
    if (context->IsUseFullLinkTimeout()) {
      error += ", request fulllink timeout.";
      result.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR);
    } else {
      error += ", request timeout.";
      result.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR);
    }
    result.SetErrorMessage(error);

    TRPC_LOG_ERROR(error);
    context->SetStatus(std::move(result));

    return true;
  }

  return false;
}

const std::string& ServiceProxy::GetServiceName() { return service_name_; }

void ServiceProxy::InitFilters() {
  // register service filters to filter controller
  for (auto& filter_name : option_->service_filters) {
    auto filter = FilterManager::GetInstance()->GetMessageClientFilter(filter_name);
    // Allow the use of naming conventions such as 'xxx_selector' to configure addressing filters, in order to meet
    // potential future naming conventions for selector filters.
    if (!filter) {
      filter = FilterManager::GetInstance()->GetMessageClientFilter(
          std::string(util::TrimSuffixStringView(filter_name, "_selector")));
    }
    if (!filter) {
      TRPC_FMT_ERROR("Can't find filter \"{}\".", filter_name);
      continue;
    }

    // If the selector_name is not configured, use the name of the selector filter to maintain compatibility.
    if (filter->Type() == FilterType::kSelector) {
      // Support naming of selector filters with the suffix '_selector', and remove the suffix to obtain the
      // corresponding selector name.
      std::string selector_name = std::string(util::TrimSuffixStringView(filter->Name(), "_selector"));
      if (option_->selector_name.empty()) {
        option_->selector_name = selector_name;
      }
      // There can only be one selector plugin in total under the selector filter and selector_name configuration.
      TRPC_CHECK_EQ(selector_name, option_->selector_name, "multiple selector specified in config.");
    }

    AddServiceFilter(filter);
  }
}

void ServiceProxy::InitSelectorFilter() {
  if (!NeedSelector()) {
    return;
  }
  if (option_->selector_name.empty()) {
    option_->selector_name = kDefaultSelectorName;
  }

  // Attempt to use selector filter names with the suffix '_selector' firstly.
  auto selector_filter = FilterManager::GetInstance()->GetMessageClientFilter(option_->selector_name + "_selector");
  if (!selector_filter) {
    selector_filter = FilterManager::GetInstance()->GetMessageClientFilter(option_->selector_name);
  }

  if (!selector_filter) {
    TRPC_FMT_ERROR("Can't find selector filter \"{}\".", option_->selector_name);
    return;
  }

  // Only take effect when no selector filter has been added.
  // If an selector filter has already been added in 'InitFilters', adding it again to the FilterController should have
  // no effect.
  AddServiceFilter(selector_filter);
}

void ServiceProxy::InitServiceNameInfo() {
  if (option_->selector_name == "direct" || option_->selector_name == "domain") {
    service_name_ = option_->name;
  } else {
    service_name_ = option_->target;
  }
}

void ServiceProxy::UnaryTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp) {
  NoncontiguousBuffer req_msg_buf;
  if (TRPC_UNLIKELY(!codec_->ZeroCopyEncode(context, req, req_msg_buf))) {
    std::string error("service name:");
    error += GetServiceName();
    error += ", request encode failed.";

    TRPC_LOG_ERROR(error);

    Status result;
    result.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
    result.SetErrorMessage(error);

    context->SetStatus(std::move(result));

    return;
  }
  CTransportReqMsg req_msg;
  req_msg.send_data = std::move(req_msg_buf);
  req_msg.context = context;

  CTransportRspMsg rsp_msg;
  int ret = transport_->SendRecv(&req_msg, &rsp_msg);
  if (ret == 0) {
    rsp = std::any_cast<ProtocolPtr&&>(std::move(rsp_msg.msg));
    return;
  }

  std::string error("service name:");
  error += GetServiceName();
  error += ", transport name:";
  error += transport_->Name();
  error += ", sendrcv failed";
  HandleError(context, ret, std::move(error));
}

void ServiceProxy::HandleError(const ClientContextPtr& context, int ret, std::string&& err_msg) {
  bool is_timeout = false;
  if (ret == TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR) {
    is_timeout = true;
    if (context->IsUseFullLinkTimeout()) {
      ret = TrpcRetCode::TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR;
    }
  }

  err_msg += ", ret:";
  err_msg += std::to_string(ret);

  TRPC_LOG_ERROR(err_msg);

  Status result;
  result.SetFrameworkRetCode(ret);
  result.SetErrorMessage(err_msg);

  context->SetStatus(std::move(result));

  // call the user-defined timeout handling function when invoke timeout
  if (is_timeout && option_->proxy_callback.client_timeout_handle_function) {
    option_->proxy_callback.client_timeout_handle_function(context);
  }
}

Future<ProtocolPtr> ServiceProxy::AsyncUnaryTransportInvoke(const ClientContextPtr& context,
                                                            const ProtocolPtr& req_protocol) {
  NoncontiguousBuffer req_msg_buf;
  if (TRPC_UNLIKELY(!codec_->ZeroCopyEncode(context, req_protocol, req_msg_buf))) {
    std::string error("service name:");
    error += GetServiceName();
    error += ", request encode failed.";

    TRPC_LOG_ERROR(error);

    Status result;
    result.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
    result.SetErrorMessage(error);

    context->SetStatus(std::move(result));

    RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);

    return MakeExceptionFuture<ProtocolPtr>(CommonException(error.c_str(), TrpcRetCode::TRPC_CLIENT_ENCODE_ERR));
  }

  auto* req_msg = object_pool::New<CTransportReqMsg>();
  req_msg->send_data = std::move(req_msg_buf);
  req_msg->context = context;

  return transport_->AsyncSendRecv(req_msg).Then([context, req_msg, this](Future<CTransportRspMsg>&& fut) mutable {
    // generate statistics of backup request
    ProxyStatistics(context);

    RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);
    TRPC_LOG_TRACE("AsyncUnaryInvoke end");

    // If backup request is used, the req_msg will be deleted in the transport layer.
    if (!context->IsBackupRequest()) {
      object_pool::Delete(req_msg);
    }

    if (fut.IsReady()) {
      auto rsp_msg = fut.GetValue0();
      ProtocolPtr rsp = std::any_cast<ProtocolPtr&&>(std::move(rsp_msg.msg));
      return MakeReadyFuture<ProtocolPtr>(std::move(rsp));
    }

    Exception ex = fut.GetException();

    std::string error("service name:");
    error += GetServiceName();
    error += ", transport name:";
    error += transport_->Name();
    error += ", ex:";
    error += ex.what();
    HandleError(context, ex.GetExceptionCode(), std::move(error));

    return MakeExceptionFuture<ProtocolPtr>(ex);
  });
}

void ServiceProxy::SetCodec(const std::string& codec_name) {
  auto codec = ClientCodecFactory::GetInstance()->Get(codec_name);

  if (codec != nullptr) {
    codec_ = codec;
    option_->codec_name = codec_name;
  } else {
    TRPC_FMT_ERROR("service name:{}|codec name:{}|not found.", GetServiceName(), codec_name);
    assert(0 && "codec must be seted");
  }
}

void ServiceProxy::InitTransport() {
  ClientTransport::Options options;
  options.trans_info = ProxyOptionToTransInfo();
  options.thread_model = GetThreadModel();

  // Prefer the use of user-defined transport
  if (option_->proxy_callback.create_transport_function != nullptr) {
    transport_ = option_->proxy_callback.create_transport_function();
    TRPC_ASSERT(transport_->Init(std::move(options)) == 0);
    return;
  }

  if (options.thread_model->Type() == kFiber) {
    transport_ = std::make_unique<FiberTransport>();
    TRPC_ASSERT(transport_->Init(std::move(options)) == 0);
  } else {
    transport_ = std::make_unique<FutureTransport>();
    TRPC_ASSERT(transport_->Init(std::move(options)) == 0);
  }
}

void ServiceProxy::SetServiceProxyOptionInner(const std::shared_ptr<ServiceProxyOption>& option) {
  option_ = option;

  SetCodec(option->codec_name);

  // Check if connection complex can be used by codec and configuration.
  // Additionally, connection complex and pipeline cannot be used simultaneously.
  if (option->is_conn_complex && codec_->IsComplex()) {
    option_->is_conn_complex = true;
    option_->support_pipeline = false;
  } else {
    option_->is_conn_complex = false;
    option_->support_pipeline = SupportPipeline(option);
  }

  InitTransport();

  PrepareStatistics(option->name);

  InitFilters();

  // Init selector filter, the selector_name configuration option will be used.
  // It should be executed after 'InitFilters'.
  InitSelectorFilter();

  // Init the service routing name.
  // It should be executed after 'InitSelectorFilter'.
  InitServiceNameInfo();
}

void ServiceProxy::PrepareStatistics(const std::string& service_name) {
  if (!backup_retries_ || !backup_retries_succ_) {
    auto data = FrameStats::GetInstance()->GetBackupRequestStats().GetData(service_name);
    backup_retries_ = std::move(data.retries);
    backup_retries_succ_ = std::move(data.retries_success);
  }
}

ThreadModel* ServiceProxy::GetThreadModel() {
  if (thread_model_ != nullptr) {
    return thread_model_;
  }

  if (!option_->threadmodel_instance_name.empty()) {
    thread_model_ = ThreadModelManager::GetInstance()->Get(option_->threadmodel_instance_name);
  } else {
    TRPC_LOG_DEBUG("service name:" << option_->name << ", thread_model instance name is empty. Try fallback policys.");
    if (runtime::IsInFiberRuntime()) {
      thread_model_ = fiber::GetFiberThreadModel();
    } else {
      // Support the configuration of the thread model as 'default'.
      // First, search for the merged thread model, then search for the separated thread model.
      thread_model_ = merge::RandomGetMergeThreadModel();
      if (thread_model_ == nullptr) {
        thread_model_ = separate::RandomGetSeparateThreadModel();
      }
    }
  }

  TRPC_ASSERT(thread_model_ != nullptr && "service proxy thread_model is nullptr");

  return thread_model_;
}

void ServiceProxy::FillClientContext(const ClientContextPtr& context) {
  // Set the start time of invoke, to avoid the value being unset when no metrics filter is configured.
  context->SetBeginTimestampUs(trpc::time::GetMicroSeconds());

  // Set the ServiceProxy option parameters to the context for use by the selector filter during route selection.
  context->SetServiceProxyOption(option_.get());

  // Set unique request id
  if (TRPC_LIKELY(!context->IsSetRequestId())) {
    context->SetRequestId(unique_id.GenerateId());
  }

  if (option_->timeout == UINT32_MAX && context->GetTimeout() == UINT32_MAX) {
    // Provide compatibility handling. If the timeout duration is not set, set it to the default value of 5000.
    context->SetTimeout(5000);
  } else {
    if (!context->IsIgnoreProxyTimeout()) {
      // Use the smaller timeout between the context and option_.
      if (option_->timeout < context->GetTimeout()) {
        context->SetTimeout(option_->timeout);
      }
    }
  }

  if (context->IsBackupRequest()) {
    // The timeout value for the backup request must be greater than the delay value, otherwise it will degrade to a
    // normal unary call.
    uint32_t delay = context->GetBackupRequestRetryInfo()->delay;
    if (TRPC_UNLIKELY(context->GetTimeout() <= delay)) {
      TRPC_FMT_ERROR_EVERY_SECOND("timeout {} <= delay {}, can not send as backup-request", context->GetTimeout(),
                                  delay);
      context->CancelBackupRequest();
    }
  }

  if (context->GetCallerName().empty()) {
    context->SetCallerName(option_->caller_name);
  }

  if (context->GetCalleeName().empty()) {
    context->SetCalleeName(option_->callee_name);
  }
}

ConnectionType ServiceProxy::GetClientConnectType() {
  ConnectionType conn_type = ConnectionType::kTcpLong;
  if (option_->network == "tcp") {
    if (option_->conn_type == "short") {
      conn_type = ConnectionType::kTcpLong;
    } else if (option_->conn_type == "long") {
      conn_type = ConnectionType::kTcpLong;
    }
  } else if (option_->network == "udp") {
    conn_type = ConnectionType::kUdp;
  }
  return conn_type;
}

TransInfo ServiceProxy::ProxyOptionToTransInfo() {
  TransInfo trans_info;

  trans_info.conn_type = GetClientConnectType();
  trans_info.connection_idle_timeout = option_->idle_time;
  trans_info.request_timeout_check_interval = option_->request_timeout_check_interval;
  trans_info.is_reconnection = option_->is_reconnection;
  trans_info.allow_reconnect = option_->allow_reconnect;
  trans_info.connect_timeout = option_->connect_timeout;
  trans_info.max_conn_num = option_->max_conn_num;
  trans_info.max_packet_size = option_->max_packet_size;
  trans_info.recv_buffer_size = option_->recv_buffer_size;
  trans_info.send_queue_capacity = option_->send_queue_capacity;
  trans_info.send_queue_timeout = option_->send_queue_timeout;
  trans_info.is_complex_conn = option_->is_conn_complex;
  trans_info.support_pipeline = option_->support_pipeline;
  trans_info.fiber_pipeline_connector_queue_size = option_->fiber_pipeline_connector_queue_size;
  trans_info.protocol = option_->codec_name;
  trans_info.fiber_connpool_shards = option_->fiber_connpool_shards;

  // set the callback function
  trans_info.conn_close_function = option_->proxy_callback.conn_close_function;
  trans_info.conn_establish_function = option_->proxy_callback.conn_establish_function;
  trans_info.rsp_dispatch_function = option_->proxy_callback.rsp_dispatch_function;
  trans_info.set_socket_opt_function = option_->proxy_callback.set_socket_opt_function;

  trans_info.checker_function = [this](const ConnectionPtr& conn, NoncontiguousBuffer& in,
                                       std::deque<std::any>& out) -> int {
    return codec_->ZeroCopyCheck(conn, in, out);
  };

  trans_info.rsp_decode_function = [this](std::any&& in, ProtocolPtr& out) -> bool {
    out = codec_->CreateResponsePtr();
    if (codec_->ZeroCopyDecode(nullptr, std::move(in), out)) {
      return true;
    }
    return false;
  };

  ThreadModel* thread_model = GetThreadModel();
  TRPC_ASSERT(thread_model != nullptr && "can not get thread model");

  // By default, use the dispatching strategy defined by the framework for handling response packets. If you want to use
  // another dispatching strategy, you can override this function.
  if (!trans_info.rsp_dispatch_function) {
    trans_info.rsp_dispatch_function = [this, thread_model](object_pool::LwUniquePtr<MsgTask>&& task) {
      TRPC_ASSERT(thread_model && "can not get thread model");

      auto* req_msg = static_cast<CTransportReqMsg*>(task->param);

      TRPC_ASSERT(req_msg->extend_info);
      auto& client_extend_info = req_msg->extend_info;
      int dst_thread_model_id = thread_model->GroupId();
      // The thread that initiates the request is an internal thread of the framework.
      if (client_extend_info->dispatch_info.src_thread_id >= 0) {
        // The thread model for request initiation and processing is the same.
        if (dst_thread_model_id == client_extend_info->dispatch_info.src_thread_model_id) {
          // If the synchronous calling method is used, because the handling thread is in a waiting state and unable to
          // process tasks, the current task will be executed directly by the IO thread.
          if (client_extend_info->is_blocking_invoke) {
            task->handler();
            return true;
          }

          // The response will be handled either by the thread that initiated the request or by a user-specified thread.
          task->dst_thread_key = client_extend_info->dispatch_info.dst_thread_key < 0
                                     ? client_extend_info->dispatch_info.src_thread_id
                                     : client_extend_info->dispatch_info.dst_thread_key;
        } else {
          // The thread model for request initiation and processing is different.
          task->dst_thread_key = client_extend_info->dispatch_info.dst_thread_key;
        }
        task->group_id = dst_thread_model_id;

        return thread_model->SubmitHandleTask(task.Leak());
      } else {
        // If the request is initiated by a user thread, the response processing will be executed directly.
        task->handler();
      }
      return true;
    };
  }

  trans_info.run_client_filters_function = [this](FilterPoint filter_point, CTransportReqMsg* msg) {
    return RunClientFilters(filter_point, msg);
  };

  trans_info.run_io_filters_function = [this](FilterPoint filter_point, const std::any& msg) {
    return RunIoFilters(filter_point, msg);
  };

  // Set SSL/TLS context and options for client.
#ifdef TRPC_BUILD_INCLUDE_SSL
  //
  // Init SSL context and options.
  // Note: ssl_ctx also indicates ssl is enable or not,
  // -- enable ssl if ssl_ctx is not nullptr when connection was created.
  //
  if (option_->ssl_config.enable) {
    // Init SSL options
    ssl::ClientSslOptions ssl_options;
    TRPC_ASSERT(ssl::InitClientSslOptions(option_->ssl_config, &ssl_options));

    // Init SSL context
    ssl::SslContextPtr ssl_ctx = MakeRefCounted<ssl::SslContext>();
    TRPC_ASSERT(ssl_ctx->Init(ssl_options));

    trans_info.ssl_options = std::move(ssl_options);
    trans_info.ssl_ctx = std::move(ssl_ctx);
  } else {
    trans_info.ssl_ctx = nullptr;
  }
#endif

  return trans_info;
}

void ServiceProxy::Stop() {
  if (transport_ != nullptr) {
    transport_->Stop();
  }

  backup_retries_.reset();
  backup_retries_succ_.reset();
}

void ServiceProxy::Destroy() {
  if (transport_ != nullptr) {
    transport_->Destroy();
  }
}

void ServiceProxy::AddServiceFilter(const MessageClientFilterPtr& filter) {
  // Each service proxy prefers to use a separate filter to avoid sharing data with other service proxies.
  auto& filter_configs = option_->service_filter_configs;
  auto iter = filter_configs.find(filter->Name());
  std::any param;
  if (iter != filter_configs.end()) {
    param = iter->second;
  }
  auto new_filter = filter->Create(param);
  if (new_filter != nullptr) {
    new_filter->Init();
    filter_controller_.AddMessageClientFilter(new_filter);
  } else {
    filter_controller_.AddMessageClientFilter(filter);
  }
}

Status ServiceProxy::OnewayInvoke(const ClientContextPtr& context, const ProtocolPtr& req) {
  TRPC_LOG_TRACE("OnewayInvoke begin");

  if (CheckTimeout(context)) {
    return context->GetStatus();
  }

  int filter_ret = RunFilters(FilterPoint::CLIENT_PRE_SEND_MSG, context);
  if (filter_ret == 0) {
    OnewayTransportInvoke(context, req);
  }
  RunFilters(FilterPoint::CLIENT_POST_RECV_MSG, context);

  TRPC_LOG_TRACE("OnewayInvoke end");

  return context->GetStatus();
}

void ServiceProxy::OnewayTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req) {
  NoncontiguousBuffer req_msg_buf;
  if (TRPC_UNLIKELY(!codec_->ZeroCopyEncode(context, req, req_msg_buf))) {
    std::string error("service name:");
    error += GetServiceName();
    error += ", request encode failed.";

    TRPC_LOG_ERROR(error);

    Status result;
    result.SetFrameworkRetCode(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR);
    result.SetErrorMessage(error);

    context->SetStatus(std::move(result));

    return;
  }
  auto* req_msg = object_pool::New<CTransportReqMsg>();
  req_msg->send_data = std::move(req_msg_buf);
  req_msg->context = context;

  int ret = transport_->SendOnly(req_msg);
  if (ret == 0) {
    return;
  }

  std::string error("service name:");
  error += GetServiceName();
  error += ", transport name:";
  error += transport_->Name();
  error += ", oneway send failed";

  HandleError(context, ret, std::move(error));
}

int ServiceProxy::RunFilters(const FilterPoint& point, const ClientContextPtr& context) {
  auto filter_status = filter_controller_.RunMessageClientFilters(point, context);

  if (filter_status == FilterStatus::REJECT) {
    TRPC_LOG_ERROR("service name:" << GetServiceName() << ", filter point:" << int(point) << ", execute failed.");
    return -1;
  }
  return 0;
}

void ServiceProxy::ProxyStatistics(const ClientContextPtr& ctx) {
  auto* retry_info = ctx->GetBackupRequestRetryInfo();
  if (retry_info) {
    if (retry_info->resend_count > 0 && backup_retries_) {
      backup_retries_->Add(retry_info->resend_count);
    }
    if (retry_info->succ_rsp_node_index > 0 && backup_retries_succ_) {
      backup_retries_succ_->Add(1);
    }
  }
}

bool ServiceProxy::SelectTarget(const ClientContextPtr& context) {
  FilterStatus status = FilterStatus::REJECT;
  auto selector_filter = FilterManager::GetInstance()->GetMessageClientFilter(option_->selector_name);
  if (selector_filter) {
    selector_filter->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  }

  return context->GetStatus().OK();
}

stream::StreamReaderWriterProviderPtr ServiceProxy::SelectStreamProvider(const ClientContextPtr& context,
                                                                         void* rpc_reply_msg) {
  // Currently, only the trpc or http protocol supports streaming RPC.
  TRPC_ASSERT(codec_->Name() == "trpc" || codec_->Name() == "http");
  TRPC_ASSERT(thread_model_ != nullptr);

  if (context->GetResponse() == nullptr) {
    context->SetResponse(codec_->CreateResponsePtr());
  }

  FillClientContext(context);

  // Get one address of peer server
  if (!SelectTarget(context)) {
    TRPC_LOG_ERROR("select target failed: " << context->GetStatus().ToString());
    return stream::CreateErrorStreamProvider(context->GetStatus());
  }

  // Set stream option
  stream::StreamOptions stream_options{};
  stream_options.context.context = context;
  stream_options.connection_id = context->GetFixedConnectorId();
  stream_options.rpc_reply_msg = rpc_reply_msg;

  // create stream
  auto stream_provider = transport_->CreateStream(context->GetNodeAddr(), std::move(stream_options));
  if (!stream_provider) {
    auto status = Status{TrpcRetCode::TRPC_STREAM_CLIENT_NETWORK_ERR, 0, "failed to create stream"};
    TRPC_LOG_ERROR("create stream failed: " << status.ToString());
    context->SetStatus(status);
    return stream::CreateErrorStreamProvider(status);
  }

  return stream_provider;
}

bool ServiceProxy::SupportPipeline(const std::shared_ptr<ServiceProxyOption>& option) {
  if (!option->support_pipeline) {
    return false;
  }
  // Short connections and UDP do not support pipeline.
  if (GetClientConnectType() != ConnectionType::kTcpLong) {
    return false;
  }
  return true;
}

FilterStatus ServiceProxy::RunClientFilters(FilterPoint filter_point, CTransportReqMsg* msg) {
  msg->context->SetRecvTimestampUs(trpc::time::GetMicroSeconds());

  return filter_controller_.RunMessageClientFilters(filter_point, msg->context);
}

FilterStatus ServiceProxy::RunIoFilters(const FilterPoint& point, const std::any& msg) noexcept {
  if (!msg.has_value()) {
    return FilterStatus::CONTINUE;
  }

  try {
    const auto& context = std::any_cast<const ClientContextPtr&>(msg);
    context->SetSendTimestampUs(trpc::time::GetMicroSeconds());

    return filter_controller_.RunMessageClientFilters(point, context);
  } catch (std::bad_any_cast& ex) {
    TRPC_LOG_ERROR("RunIoFilters throw exception: " << ex.what());
    return FilterStatus::CONTINUE;
  }
}

std::optional<uint64_t> ServiceProxy::GetOrCreateConnector(const PreallocationOption& preallocate_option) {
  TRPC_ASSERT(!runtime::IsInFiberRuntime() && "only support future now");

  // If no node is specified, get one node of the service.
  if (preallocate_option.node_addr.ip.empty() || preallocate_option.node_addr.port == 0) {
    ClientContextPtr ctx = MakeRefCounted<ClientContext>(this->GetClientCodec());
    FillClientContext(ctx);
    if (!SelectTarget(ctx)) {
      TRPC_LOG_ERROR("select target failed: " << ctx->GetStatus().ToString());
      return std::nullopt;
    }
    PreallocationOption new_preallocate_option = preallocate_option;
    new_preallocate_option.node_addr = ctx->GetNodeAddr();
    return transport_->GetOrCreateConnector(new_preallocate_option);
  }

  return transport_->GetOrCreateConnector(preallocate_option);
}

bool ServiceProxy::ReleaseFixedConnector(uint64_t connector_id) {
  TRPC_ASSERT(!runtime::IsInFiberRuntime() && "only support future now");
  return transport_->ReleaseFixedConnector(connector_id);
}

void ServiceProxy::Disconnect(const std::string& target_ip) {
  TRPC_ASSERT(!runtime::IsInFiberRuntime() && "only support future now");

  return transport_->Disconnect(target_ip);
}

namespace {
// convert a string of ip_ports into an array of endpoint objects
void ConvertEndpointInfo(const std::string& ip_ports, std::vector<TrpcEndpointInfo>& vec_endpoint) {
  std::vector<std::string> vec = util::SplitString(ip_ports, ',');
  for (auto const& name : vec) {
    TrpcEndpointInfo endpoint;
    // add to vec_endpoint when parse endpoints successfully
    if (util::ParseHostPort(name, endpoint.host, endpoint.port, endpoint.is_ipv6)) {
      vec_endpoint.emplace_back(endpoint);
    }
  }
}
}  // namespace

void ServiceProxy::SetEndpointInfo(const std::string& endpoint_info) {
  if (GetServiceName().empty()) {
    TRPC_LOG_ERROR("Invalid use:service_name is empty");
    assert(!GetServiceName().empty());
  }

  RouterInfo info;
  info.name = GetServiceName();
  ConvertEndpointInfo(endpoint_info, info.info);
  if (info.info.size() <= 0) {
    TRPC_LOG_ERROR("Service endpoint info of " << GetServiceName()
                                               << " is invalided, endpoint_info = " << endpoint_info);
    return;
  }

  if (option_->selector_name.empty()) {
    // determine if endpoint_info is in domain name format
    util::AddrType type = util::GetAddrType(info.info[0].host);
    std::string selector_name = (type == util::AddrType::ADDR_DOMAIN ? "domain" : "direct");
    TRPC_LOG_ERROR("Selector name is no set, set it to : " << selector_name);
    option_->selector_name = selector_name;
    assert(!option_->selector_name.empty());
  }

  auto selector = SelectorFactory::GetInstance()->Get(option_->selector_name);
  assert(selector != nullptr);
  selector->SetEndpoints(&info);
}

}  // namespace trpc
