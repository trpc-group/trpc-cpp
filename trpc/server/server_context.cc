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

#include "trpc/server/server_context.h"

#include <algorithm>
#include <string>
#include <unordered_map>

#include "trpc/common/config/trpc_config.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/coroutine/fiber_local.h"
#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/server/service.h"
#include "trpc/util/time.h"

namespace trpc {

ServerContext::ServerContext() { FrameStats::GetInstance()->GetServerStats().AddReqConcurrency(); }

ServerContext::~ServerContext() {
  if (rpc_method_handler_ != nullptr) {
    GetRpcMethodHandler()->DestroyReqObj(this);
    GetRpcMethodHandler()->DestroyRspObj(this);
    rpc_method_handler_ = nullptr;
  }

  auto& server_stats = FrameStats::GetInstance()->GetServerStats();

  server_stats.SubReqConcurrency();
  server_stats.AddReqCount();
  if (status_.GetFrameworkRetCode() != TrpcRetCode::TRPC_INVOKE_SUCCESS) {
    server_stats.AddFailedReqCount();
  }

  uint64_t cost = trpc::time::GetMilliSeconds() - (metrics_info_.recv_timestamp_us / 1000);
  if (cost > 0) {
    server_stats.AddReqDelay(cost);
  }
}

bool ServerContext::IsDyeingMessage() const { return (GetMessageType() & TrpcMessageType::TRPC_DYEING_MESSAGE) != 0; }

std::string ServerContext::GetDyeingKey() { return GetDyeingKey(TRPC_DYEING_KEY); }

std::string ServerContext::GetDyeingKey(const std::string& key) {
  auto* pb_trans_info = GetMutablePbReqTransInfo();
  if (pb_trans_info) {
    auto it = pb_trans_info->find(key);
    if (it != pb_trans_info->end()) {
      return it->second;
    }
  }

  return "";
}

void ServerContext::SetDyeingKey(const std::string& value) { SetDyeingKey(TRPC_DYEING_KEY, value); }

void ServerContext::SetDyeingKey(const std::string& key, const std::string& value) {
  req_msg_->SetKVInfo(key, value);

  invoke_info_.message_type |= TrpcMessageType::TRPC_DYEING_MESSAGE;
}

void ServerContext::SendUnaryResponse(const Status& status, google::protobuf::Message* pb) {
  void* rsp_data = static_cast<void*>(pb);
  SendUnaryResponse(status, rsp_data, serialization::kPbMessage);
}

void ServerContext::SendUnaryResponse(const Status& status, flatbuffers::trpc::MessageFbs* fbs) {
  void* rsp_data = static_cast<void*>(fbs);
  SendUnaryResponse(status, rsp_data, serialization::kFlatBuffers);
}

void ServerContext::SendUnaryResponse(const Status& status, rapidjson::Document* json) {
  void* rsp_data = static_cast<void*>(json);
  SendUnaryResponse(status, rsp_data, serialization::kRapidJson);
}

void ServerContext::SendUnaryResponse(const Status& status, NoncontiguousBuffer* buffer) {
  void* rsp_data = static_cast<void*>(buffer);
  SendUnaryResponse(status, rsp_data, serialization::kNonContiguousBufferNoop);
}

void ServerContext::SendUnaryResponse(const Status& status, std::string* str) {
  void* rsp_data = static_cast<void*>(str);
  SendUnaryResponse(status, rsp_data, serialization::kStringNoop);
}

void ServerContext::SendUnaryResponse(const Status& status, void* rsp_data, serialization::DataType type) {
  TRPC_ASSERT(!IsResponse());

  if (rsp_data != rsp_data_) {
    if (TRPC_LIKELY(rpc_method_handler_ != nullptr)) {
      rpc_method_handler_->DestroyRspObj(this);
    }
  }
  SetResponseData(rsp_data);

  RefPtr ref(ref_ptr, this);
  GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, ref);

  SetResponseData(nullptr);

  if (TRPC_UNLIKELY(!status.OK())) {
    return SendUnaryResponse(status);
  }

  // serialize
  serialization::SerializationType serialization_type = GetRspEncodeType();
  serialization::SerializationFactory* serializationfactory = serialization::SerializationFactory::GetInstance();
  serialization::Serialization* serialization = serializationfactory->Get(serialization_type);
  if (TRPC_UNLIKELY(serialization == nullptr)) {
    std::string err_msg = "not support serialization_type:";
    err_msg += std::to_string(serialization_type);

    TRPC_LOG_ERROR(err_msg);
    return HandleEncodeErrorResponse(std::move(err_msg));
  }

  NoncontiguousBuffer data;
  auto ret = serialization->Serialize(type, rsp_data, &data);
  if (TRPC_UNLIKELY(!ret)) {
    std::string err_msg = "serialize response data failed, serialization_type:";
    err_msg += std::to_string(serialization_type);

    TRPC_FMT_ERROR(err_msg);
    return HandleEncodeErrorResponse(std::move(err_msg));
  }

  // compress
  auto compress_type = GetRspCompressType();
  ret = compressor::CompressIfNeeded(compress_type, data, GetRspCompressLevel());
  if (TRPC_UNLIKELY(!ret)) {
    std::string err_msg = "compress response data failed, compress_type:";
    err_msg += std::to_string(compress_type);

    TRPC_FMT_ERROR(err_msg);
    return HandleEncodeErrorResponse(std::move(err_msg));
  }

  rsp_msg_->SetNonContiguousProtocolBody(std::move(data));

  if (rpc_method_handler_ != nullptr) {
    rpc_method_handler_->DestroyReqObj(this);
    rpc_method_handler_ = nullptr;
  }

  SendUnaryResponse(status);
}

void ServerContext::SendUnaryResponse(const Status& status) {
  status_ = status;

  // Regardless of the previous status, check if the server has timed out before sending the response, maintaining the
  // same logic as synchronous response.
  CheckHandleTimeout();

  RefPtr ref(ref_ptr, this);
  GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, ref);

  // one way calls do not require return packets
  if (GetCallType() == kOnewayCall) {
    return;
  }

  // encode
  NoncontiguousBuffer send_data;
  bool ret = GetServerCodec()->ZeroCopyEncode(ref, rsp_msg_, send_data);
  if (!ret) {
    TRPC_FMT_ERROR("response encode failed, ip: {}", GetIp());
    return;
  }

  auto* send_msg = object_pool::New<STransportRspMsg>();
  send_msg->context = std::move(ref);
  send_msg->buffer = std::move(send_data);

  service_->SendMsg(send_msg);
}

void ServerContext::SendTransparentResponse(const Status& status, NoncontiguousBuffer&& bin_rsp) {
  if (status.OK()) {
    rsp_msg_->SetNonContiguousProtocolBody(std::move(bin_rsp));
  }

  SendUnaryResponse(status);
}

Status ServerContext::SendResponse(NoncontiguousBuffer&& buffer) {
  RefPtr ref(ref_ptr, this);

  GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_POST_RPC_INVOKE, ref);
  GetFilterController().RunMessageServerFilters(FilterPoint::SERVER_PRE_SEND_MSG, ref);

  auto* send_msg = object_pool::New<STransportRspMsg>();
  send_msg->context = std::move(ref);
  send_msg->buffer = std::move(buffer);

  return Status{service_->SendMsg(send_msg), 0, ""};
}

void ServerContext::HandleEncodeErrorResponse(std::string&& err_msg) {
  Status encode_error;
  if (GetServerCodec() != nullptr) {
    encode_error.SetFrameworkRetCode(GetServerCodec()->GetProtocolRetCode(codec::ServerRetCode::ENCODE_ERROR));
  } else {
    TRPC_LOG_ERROR("codec is nullptr, use default retcode");
    encode_error.SetFrameworkRetCode(TrpcRetCode::TRPC_SERVER_ENCODE_ERR);
  }
  encode_error.SetErrorMessage(std::move(err_msg));

  SendUnaryResponse(encode_error);
}

void ServerContext::SetRealTimeout() {
  bool use_fulllink = false;
  if (invoke_info_.timeout != UINT32_MAX) {
    use_fulllink = true;
  }

  if (service_) {
    auto& option = service_->GetServiceAdapterOption();
    if (!option.disable_request_timeout) {
      use_fulllink = use_fulllink && (invoke_info_.timeout <= option.timeout);
      invoke_info_.timeout = std::min(option.timeout, invoke_info_.timeout);
    } else {
      invoke_info_.timeout = option.timeout;
      use_fulllink = false;
    }
  }

  SetStateFlag(use_fulllink, kIsUseFulllinkTimeoutMask);
}

bool ServerContext::CheckHandleTimeout() {
  uint64_t nowms = trpc::time::GetMilliSeconds();
  if (GetRecvTimestamp() + GetTimeout() <= nowms) {
    if (GetStateFlag(kIsUseFulllinkTimeoutMask)) {
      status_.SetFrameworkRetCode(
          GetServerCodec()->GetProtocolRetCode(trpc::codec::ServerRetCode::FULL_LINK_TIMEOUT_ERROR));
      status_.SetErrorMessage("request handle fulllink timeout.");
    } else {
      status_.SetFrameworkRetCode(GetServerCodec()->GetProtocolRetCode(trpc::codec::ServerRetCode::TIMEOUT_ERROR));
      status_.SetErrorMessage("request handle timeout.");
    }

    ServiceTimeoutHandleFunction& timeout_handler = GetService()->GetServiceTimeoutHandleFunction();
    if (timeout_handler) {
      RefPtr ref(ref_ptr, this);
      timeout_handler(ref);
    }
    return true;
  }

  return false;
}

std::string ServerContext::GetLogFieldValue(uint32_t filter_id, const std::string& key) {
  auto* logging_data = GetFilterData<std::unordered_map<std::string, std::string>>(filter_id);
  if (logging_data) {
    auto it = logging_data->find(key);
    if (it != logging_data->end()) {
      return it->second;
    }
  }

  return "";
}

bool ServerContext::IsConnected() {
  if (service_) {
    return service_->IsConnected(net_info_.connection_id);
  }
  return false;
}

void ServerContext::CloseConnection() {
  if (service_) {
    CloseConnectionInfo conn;
    conn.connection_id = net_info_.connection_id;
    conn.client_ip = net_info_.ip;
    conn.client_port = net_info_.port;
    conn.fd = net_info_.fd;
    service_->CloseConnection(conn);
  }
}

void ServerContext::ThrottleConnection(bool set) {
  if (service_) {
    service_->ThrottleConnection(net_info_.connection_id, set);
  }
}

// Context used for storing data in a fiber environment.
FiberLocal<ServerContext*> fls_server_context;

// Context used for storing data in a regular thread environment, such as setting it in a business thread and releasing
// it when the business request processing is completed.
thread_local ServerContext* tls_server_context = nullptr;

void SetLocalServerContext(const ServerContextPtr& context) {
  // Set to fiberLocal in a fiber environment, and set to threadLocal in a regular thread environment.
  if (trpc::fiber::detail::GetCurrentFiberEntity()) {
    *fls_server_context = context.Get();
  } else {
    tls_server_context = context.Get();
  }
}

ServerContextPtr GetLocalServerContext() {
  // Retrieve from fiberLocal in a fiber environment, and retrieve from threadLocal in a regular thread environment.
  if (trpc::fiber::detail::GetCurrentFiberEntity()) {
    return RefPtr(ref_ptr, *fls_server_context);
  }
  return RefPtr(ref_ptr, tls_server_context);
}

}  // namespace trpc
