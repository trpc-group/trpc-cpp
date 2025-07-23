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

#include "trpc/common/status.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class ServerContext;
using ServerContextPtr = RefPtr<ServerContext>;

namespace http {

// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/include/seastar/http/handlers.hh.

/// @brief Handler holds the main logic for serving the HTTP request.
/// All handlers inherit from the HandlerBase and implement the Handle method.
class HandlerBase {
 public:
  explicit HandlerBase(bool is_stream = false) : is_stream_(is_stream) {}

  virtual ~HandlerBase() = default;

  /// @brief Serving the HTTP request. All HTTP handlers must implement this method.
  /// @param path is the path of URL used in this method.
  /// @param req is the original HTTP request.
  /// @param rsp is the response will be send to client.
  virtual trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, trpc::http::RequestPtr req,
                              trpc::http::Response* rsp) = 0;

  /// @brief Reports whether is streaming handler.
  /// @private For internal use purpose only.
  bool IsStream() const { return is_stream_; }

 private:
  bool is_stream_;
};
// End of source codes that are from seastar.

}  // namespace http

}  // namespace trpc
