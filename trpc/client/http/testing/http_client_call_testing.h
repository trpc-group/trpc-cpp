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

#include <memory>

#include "trpc/client/http/http_service_proxy.h"

namespace trpc::testing {

using HttpServiceProxyPtr = std::shared_ptr<trpc::http::HttpServiceProxy>;

bool TestHttpClientUnaryRpcCall(const HttpServiceProxyPtr& proxy);
bool TestHttpClientUnaryCall(const HttpServiceProxyPtr& proxy);
bool TestHttpClientStreamingCall(const HttpServiceProxyPtr& proxy);


}  // namespace trpc::testing
