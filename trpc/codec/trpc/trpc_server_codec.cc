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

#include "trpc/codec/trpc/trpc_server_codec.h"

#include <memory>
#include <utility>

#include "trpc/codec/trpc/trpc_proto_checker.h"
#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/util/log/logging.h"

namespace trpc {

int TrpcServerCodec::ZeroCopyCheck(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  return CheckTrpcProtocolMessage(conn, in, out);
}

bool TrpcServerCodec::ZeroCopyDecode(const ServerContextPtr& context, std::any&& in, ProtocolPtr& out) {
  auto buff = std::any_cast<NoncontiguousBuffer&&>(std::move(in));
  auto* req = static_cast<TrpcRequestProtocol*>(out.get());

  bool ret = req->ZeroCopyDecode(buff);

  if (ret) {
    context->SetTimeout(req->req_header.timeout());
    context->SetCallType(req->req_header.call_type());
    context->SetRequestId(req->req_header.request_id());
    context->SetMessageType(req->req_header.message_type());
    context->SetReqEncodeType(req->req_header.content_type());
    context->SetReqCompressType(req->req_header.content_encoding());
    context->SetRequestAttachment(req->GetProtocolAttachment());

    ProtocolPtr& rsp_msg = context->GetResponseMsg();
    auto* rsp = static_cast<TrpcResponseProtocol*>(rsp_msg.get());

    context->SetRspEncodeType(context->GetReqEncodeType());
    context->SetRspCompressType(context->GetReqCompressType());

    rsp->rsp_header.set_message_type(context->GetMessageType());

    auto* rsp_trans_info = rsp->rsp_header.mutable_trans_info();
    const auto& req_trans_info = out->GetKVInfos();
    auto it = req_trans_info.begin();
    while (it != req_trans_info.end()) {
      (*rsp_trans_info)[it->first] = it->second;
      ++it;
    }
  }

  // No response if it has a decoded failure.
  if (!ret) {
    context->SetResponseWhenDecodeFail(false);
  }

  return ret;
}

bool TrpcServerCodec::ZeroCopyEncode(const ServerContextPtr& context, ProtocolPtr& in, NoncontiguousBuffer& out) {
  ProtocolPtr req_msg = context->GetRequestMsg();
  auto* req = static_cast<TrpcRequestProtocol*>(req_msg.get());
  auto* rsp = static_cast<TrpcResponseProtocol*>(in.get());

  rsp->rsp_header.set_version(req->req_header.version());
  rsp->rsp_header.set_call_type(static_cast<TrpcCallType>(context->GetCallType()));
  rsp->rsp_header.set_request_id(context->GetRequestId());

  int ret_code = context->GetStatus().GetFrameworkRetCode();
  rsp->rsp_header.set_ret(ret_code);
  rsp->rsp_header.set_func_ret(context->GetStatus().GetFuncRetCode());
  rsp->rsp_header.set_error_msg(context->GetStatus().ErrorMessage());
  rsp->rsp_header.set_message_type(req->req_header.message_type());
  rsp->rsp_header.set_content_type(context->GetRspEncodeType());
  rsp->rsp_header.set_content_encoding(context->GetRspCompressType());

  rsp->fixed_header.stream_id = context->GetStreamId();

  return rsp->ZeroCopyEncode(out);
}

ProtocolPtr TrpcServerCodec::CreateRequestObject() { return std::make_shared<TrpcRequestProtocol>(); }

ProtocolPtr TrpcServerCodec::CreateResponseObject() { return std::make_shared<TrpcResponseProtocol>(); }

bool TrpcServerCodec::Pick(const std::any& message, std::any& data) const {
  return PickTrpcProtocolMessageMetadata(message, data);
}

}  // namespace trpc
