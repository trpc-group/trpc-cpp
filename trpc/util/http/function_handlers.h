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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. seastar
// Copyright (C) 2015 Cloudius Systems
// seastar is licensed under the Apache 2.0 License.
//
//

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/util/http/handler.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class ServerContext;
using ServerContextPtr = RefPtr<ServerContext>;

namespace http {

// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/include/seastar/http/function_handlers.hh.

/// @brief ServeStringFunction is a lambda expression that gets the HTTP request as parameter and serves string as
/// response body.
using ServeStringFunction =
    std::function<trpc::Status(trpc::ServerContextPtr context, trpc::http::RequestPtr req, std::string& str)>;
using RequestFunction = ServeStringFunction;

/// @brief ServeJsonFunction is a lambda expression that gets the HTTP request as parameter and serves JSON as response
/// body.
using ServeJsonFunction =
    std::function<trpc::Status(trpc::ServerContextPtr context, trpc::http::RequestPtr req, rapidjson::Document& doc)>;
using JsonRequestFunction = ServeJsonFunction;

/// @brief HandleFunction is a lambda expression executed as HTTP handler.
using HandleFunction =
    std::function<trpc::Status(trpc::ServerContextPtr context, trpc::http::RequestPtr req, trpc::http::Response*)>;

/// @brief FuncHandler gets a lambda expression in the constructor, it will call that expression to get the result.
/// This is more suitable for simple HTTP handlers.
class FuncHandler : public HandlerBase {
 public:
  FuncHandler(HandleFunction handle_function, std::string type)
      : handle_function_(std::move(handle_function)), type_(std::move(type)) {}

  FuncHandler(const ServeStringFunction& handle_function, std::string type)
      : handle_function_(
            [handle_function](trpc::ServerContextPtr context, trpc::http::RequestPtr req, trpc::http::Response* rsp) {
              return handle_function(std::move(context), std::move(req), *rsp->GetMutableContent());
            }),
        type_(std::move(type)) {}

  explicit FuncHandler(const ServeJsonFunction& json_request_function)
      : handle_function_([json_request_function](trpc::ServerContextPtr context, trpc::http::RequestPtr req,
                                                 trpc::http::Response* rsp) {
          rapidjson::Document content;
          auto status = json_request_function(std::move(context), std::move(req), content);
          // Converts JSON document to string.
          rapidjson::StringBuffer buffer;
          rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
          content.Accept(writer);
          rsp->GetMutableContent()->append(buffer.GetString());
          return status;
        }),
        type_("json") {}

  trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, trpc::http::RequestPtr req,
                trpc::http::Response* rsp) override {
    rsp->SetContentType(type_);
    auto status = handle_function_(std::move(context), std::move(req), rsp);
    rsp->Done();
    return status;
  }

  ~FuncHandler() override = default;

 protected:
  HandleFunction handle_function_;
  std::string type_;
};
// End of source codes that are from seastar.

}  // namespace http

}  // namespace trpc
