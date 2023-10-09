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

#pragma once

#include <string>

#include "trpc/codec/server_codec_factory.h"
#include "trpc/server/make_server_context.h"
#include "trpc/server/service.h"
#include "trpc/util/chrono/time.h"

namespace trpc::testing {

ServerContextPtr MakeTestServerContext(const std::string& protocol,
                                       Service* service,
                                       std::any&& msg) {
  ServerCodecPtr codec = ServerCodecFactory::GetInstance()->Get(protocol);

  ServerContextPtr context = ::trpc::MakeServerContext();

  context->SetRecvTimestampUs(time::GetMicroSeconds());
  context->SetConnectionId(1);
  context->SetFd(1);
  context->SetNetType(ServerContext::NetType::kTcp);
  context->SetPort(12345);
  context->SetIp("127.0.0.1");
  context->SetServerCodec(codec.get());
  context->SetRequestMsg(codec->CreateRequestObject());
  context->SetResponseMsg(codec->CreateResponseObject());

  bool ret = codec->ZeroCopyDecode(context, std::move(msg), context->GetRequestMsg());
  if (!ret) {
    auto& status = context->GetStatus();
    status.SetFrameworkRetCode(GetDefaultServerRetCode(codec::ServerRetCode::DECODE_ERROR));
    status.SetErrorMessage("request header decode failed");
  }

  context->SetService(service);
  context->SetFilterController(&(service->GetFilterController()));

  return context;
}

}  // namespace trpc::testing
