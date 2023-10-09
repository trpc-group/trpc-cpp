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
#include <string>

#include "trpc/server/server_context.h"
#include "trpc/server/service.h"
#include "trpc/util/http/http_handler_groups.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/http/routes.h"

namespace trpc {

/// @brief Dispatches HTTP request to specific HTTP handler based on request routes which is initialized by user.
class HttpService : public Service {
 public:
  /// @brief Processes unary request message received by the transport and return the response message.
  /// @param[in] recv  transport request message, not owned.
  ///                  the caller of this method created it from object pool.
  ///                  the caller of this method needs to return it to the object pool.
  /// @param[out] send transport response message, if *send not nullptr, it should created by object pool.
  ///                  the caller of this method needs to return it to the object pool.
  void HandleTransportMessage(STransportReqMsg* recv, STransportRspMsg** send) noexcept override;

  /// @brief Sets routes for HTTP requests.
  /// @param func is a function for setting routes.
  void SetRoutes(const std::function<void(http::Routes& r)>& func) { func(routes_); }

  /// @brief Sets routes for HTTP requests.
  /// @param func is a function for setting routes.
  void SetRoutes(const std::function<void(http::HttpHandlerGroups r)>& func) { func(http::HttpHandlerGroups(routes_)); }

  /// @brief Gets routes of HTTP request serving.
  http::HttpRoutes& GetRoutes() { return routes_; }

 protected:
  /// @brief Dispatches request to HTTP handler.
  /// @param path is path of request URI.
  /// @param context is server side context.
  /// @param req is HTTP request.
  /// @param rsp is HTTP response.
  /// @return Returns status of handler's execution.
  Status Dispatch(const std::string& path, http::HandlerBase* handler, ServerContextPtr& context, http::RequestPtr& req,
                  http::Response& rsp);

 private:
  void Handle(const std::string& path, http::HandlerBase* handler, ServerContextPtr& context, http::RequestPtr& req,
              http::Response& rsp, STransportRspMsg** send);

  static void HandleError(ServerContextPtr& context, http::RequestPtr& req, http::Response& rsp, const Status& status);

  void CheckTimeout(const ServerContextPtr& context);

 protected:
  http::Routes routes_;
};

}  // namespace trpc
