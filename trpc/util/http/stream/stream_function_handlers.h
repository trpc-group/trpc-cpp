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

#include <functional>

#include "trpc/util/http/stream/stream_handler.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::http {

/// @brief StreamHandleFunction is a lambda expression executed as HTTP streaming handler.
using StreamHandleFunction = std::function<trpc::Status(trpc::ServerContextPtr context, RequestPtr req, Response*)>;

/// @brief StreamFuncHandler gets a lambda expression in the constructor, it will call that expression to get the result.
/// This is more suitable for simple HTTP streaming handlers.
class StreamFuncHandler : public StreamHandlerBase {
 public:
  StreamFuncHandler(const StreamHandleFunction& handle_function, std::string type)
      : handle_function_([handle_function](trpc::ServerContextPtr context, RequestPtr req, Response* rsp) {
          return handle_function(std::move(context), std::static_pointer_cast<HttpRequest>(req), rsp);
        }),
        type_(std::move(type)) {}

  Status HandleStream(const std::string& path, trpc::ServerContextPtr context, RequestPtr req, Response* rsp) override {
    rsp->SetContentType(type_);
    return handle_function_(std::move(context), std::move(req), rsp);
  }

 protected:
  StreamHandleFunction handle_function_;
  std::string type_;
};

}  // namespace trpc::http
