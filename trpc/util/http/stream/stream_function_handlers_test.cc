//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include "gtest/gtest.h"

#include "trpc/util/http/stream/stream_function_handlers.h"

namespace trpc::testing {

TEST(TestStreamFunctionHandlers, HandleStream) {
  http::StreamHandleFunction func = [](ServerContextPtr context, http::RequestPtr req, http::Response* rsp) {
    if (rsp->GetHeader("Content-Type") == "image/jpeg") {
      return kDefaultStatus;
    }

    return Status(-1, "");
  };
  http::StreamFuncHandler handler(func, "jpg");
  trpc::ServerContextPtr context;
  http::RequestPtr req;
  http::Response rsp;
  ASSERT_TRUE(handler.HandleStream("/", context, req, &rsp).OK());
}

}  // namespace trpc::testing
