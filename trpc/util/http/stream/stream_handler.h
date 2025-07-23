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

#pragma once

#include "trpc/util/http/handler.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"

namespace trpc::http {

/// @brief Handler holds the main logic for serving the HTTP streaming request.
/// All handlers inherit from the StreamHandlerBase and implement the HandleStream method.
class StreamHandlerBase : public HandlerBase {
 public:
  StreamHandlerBase() : HandlerBase(true) {}

  /// @private For internal use purpose only.
  trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, RequestPtr req, Response* rsp) final {
    try {
      return HandleStream(path, context, req, (rsp));
    } catch (...) {  // close stream and connection if stream handler throws exception.
      req->GetStream().Close();
      context->CloseConnection();
      throw;
    }
  }

  /// @brief Serving the HTTP streaming request. All HTTP streaming handlers must implement this method.
  /// @param path is the path of URL used in this method.
  /// @param req is the original HTTP request.
  /// @param rsp is the response will be send to client.
  virtual trpc::Status HandleStream(const std::string& path, trpc::ServerContextPtr context, RequestPtr req,
                                    Response* rsp) = 0;
};

}  // namespace trpc::http
