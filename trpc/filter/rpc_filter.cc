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

#include "trpc/filter/rpc_filter.h"

#include <string>

#include "trpc/client/client_context.h"
#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/server/server_context.h"
#include "trpc/util/log/logging.h"

namespace trpc {

void RpcServerFilter::operator()(FilterStatus& status, FilterPoint point,
                                 const ServerContextPtr& context) {
  void* req = context->GetRequestData();
  void* rsp = context->GetResponseData();
  Status invoke_status = kSuccStatus;

  if (point == FilterPoint::SERVER_PRE_RPC_INVOKE) {
    invoke_status = BeforeRpcInvoke(context, req, rsp);
  } else if (point == FilterPoint::SERVER_POST_RPC_INVOKE) {
    invoke_status = AfterRpcInvoke(context, req, rsp);
  }

  if (!invoke_status.OK()) {
    status = FilterStatus::REJECT;
    return;
  }

  status = FilterStatus::CONTINUE;
}

Status RpcServerFilter::BeforeRpcInvoke(ServerContextPtr context, void* req_raw, void* rsp_raw) {
  uint8_t encode_type = context->GetReqEncodeType();
  if (encode_type == TrpcContentEncodeType::TRPC_PROTO_ENCODE) {
    auto req = static_cast<PbMessage*>(req_raw);
    auto rsp = static_cast<PbMessage*>(rsp_raw);
    return BeforeRpcInvoke(context, req, rsp);
  } else if (encode_type == TrpcContentEncodeType::TRPC_FLATBUFFER_ENCODE) {
    auto req = static_cast<FbsMessage*>(req_raw);
    auto rsp = static_cast<FbsMessage*>(rsp_raw);
    return BeforeRpcInvoke(context, req, rsp);
  }
  TRPC_LOG_ERROR("Unrecognized encode type when invoking BeforeRpcInvoke: " << Name());
  return Status(TrpcRetCode::TRPC_SERVER_ENCODE_ERR, 0,
                "Unrecognized encode type at server BeforeRpcInvoke");
}

Status RpcServerFilter::AfterRpcInvoke(ServerContextPtr context, void* req_raw, void* rsp_raw) {
  uint8_t encode_type = context->GetReqEncodeType();
  if (encode_type == TrpcContentEncodeType::TRPC_PROTO_ENCODE) {
    auto req = static_cast<PbMessage*>(req_raw);
    auto rsp = static_cast<PbMessage*>(rsp_raw);
    return AfterRpcInvoke(context, req, rsp);
  } else if (encode_type == TrpcContentEncodeType::TRPC_FLATBUFFER_ENCODE) {
    auto req = static_cast<FbsMessage*>(req_raw);
    auto rsp = static_cast<FbsMessage*>(rsp_raw);
    return AfterRpcInvoke(context, req, rsp);
  }
  TRPC_LOG_ERROR("Unrecognized encode type when invoking AfterRpcInvoke: " << Name());
  return Status(TrpcRetCode::TRPC_SERVER_ENCODE_ERR, 0,
                "Unrecognized encode type at server AfterRpcInvoke");
}

void RpcClientFilter::operator()(FilterStatus& status, FilterPoint point,
                                 const ClientContextPtr& context) {
  void* req = context->GetRequestData();
  void* rsp = context->GetResponseData();
  Status invoke_status = kSuccStatus;

  if (point == FilterPoint::CLIENT_PRE_RPC_INVOKE) {
    invoke_status = BeforeRpcInvoke(context, req, rsp);
  } else if (point == FilterPoint::CLIENT_POST_RPC_INVOKE) {
    invoke_status = AfterRpcInvoke(context, req, rsp);
  }

  if (!invoke_status.OK()) {
    status = FilterStatus::REJECT;
    return;
  }

  status = FilterStatus::CONTINUE;
}

Status RpcClientFilter::BeforeRpcInvoke(ClientContextPtr context, void* req_raw, void* rsp_raw) {
  uint8_t encode_type = context->GetReqEncodeType();
  if (encode_type == TrpcContentEncodeType::TRPC_PROTO_ENCODE) {
    auto req = static_cast<PbMessage*>(req_raw);
    auto rsp = static_cast<PbMessage*>(rsp_raw);
    return BeforeRpcInvoke(context, req, rsp);
  } else if (encode_type == TrpcContentEncodeType::TRPC_FLATBUFFER_ENCODE) {
    auto req = static_cast<FbsMessage*>(req_raw);
    auto rsp = static_cast<FbsMessage*>(rsp_raw);
    return BeforeRpcInvoke(context, req, rsp);
  } else if (encode_type == TrpcContentEncodeType::TRPC_JSON_ENCODE) {
    auto req = static_cast<rapidjson::Document*>(req_raw);
    auto rsp = static_cast<rapidjson::Document*>(rsp_raw);
    return BeforeRpcInvoke(context, req, rsp);
  } else if (encode_type == TrpcContentEncodeType::TRPC_NOOP_ENCODE) {
    auto req = static_cast<std::string*>(req_raw);
    auto rsp = static_cast<std::string*>(rsp_raw);
    return BeforeRpcInvoke(context, req, rsp);
  }
  TRPC_LOG_ERROR("Unrecognized encode type when invoking AfterRpcInvoke: " << Name());
  return Status(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, 0,
                "Unrecognized encode type at client BeforeRpcInvoke");
}

Status RpcClientFilter::AfterRpcInvoke(ClientContextPtr context, void* req_raw, void* rsp_raw) {
  uint8_t encode_type = context->GetReqEncodeType();
  if (encode_type == TrpcContentEncodeType::TRPC_PROTO_ENCODE) {
    auto req = static_cast<PbMessage*>(req_raw);
    auto rsp = static_cast<PbMessage*>(rsp_raw);
    return AfterRpcInvoke(context, req, rsp);
  } else if (encode_type == TrpcContentEncodeType::TRPC_FLATBUFFER_ENCODE) {
    auto req = static_cast<FbsMessage*>(req_raw);
    auto rsp = static_cast<FbsMessage*>(rsp_raw);
    return AfterRpcInvoke(context, req, rsp);
  } else if (encode_type == TrpcContentEncodeType::TRPC_JSON_ENCODE) {
    auto req = static_cast<rapidjson::Document*>(req_raw);
    auto rsp = static_cast<rapidjson::Document*>(rsp_raw);
    return AfterRpcInvoke(context, req, rsp);
  } else if (encode_type == TrpcContentEncodeType::TRPC_NOOP_ENCODE) {
    auto req = static_cast<std::string*>(req_raw);
    auto rsp = static_cast<std::string*>(rsp_raw);
    return AfterRpcInvoke(context, req, rsp);
  }
  TRPC_LOG_ERROR("Unrecognized encode type when invoking AfterRpcInvoke: " << Name());
  return Status(TrpcRetCode::TRPC_CLIENT_ENCODE_ERR, 0,
                "Unrecognized encode type at client AfterRpcInvoke");
}

}  // namespace trpc
