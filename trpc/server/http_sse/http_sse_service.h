// trpc/server/http_sse/http_sse_service.h
//
// SSE-specialized HTTP service header.

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

/// @brief SSE-specialized HTTP service. Minimal modifications compared to HttpService:
///        detects SSE requests and enables streaming + SSE headers for handlers that are stream-capable.
class HttpSseService : public Service {
 public:
  /// Process transport message (override Service API).
  void HandleTransportMessage(STransportReqMsg* recv, STransportRspMsg** send) noexcept override;

  /// Set routes using a routes setter function (same interface as HttpService).
  void SetRoutes(const std::function<void(http::Routes& r)>& func) { func(routes_); }

  /// Set routes using handler groups helper.
  void SetRoutes(const std::function<void(http::HttpHandlerGroups r)>& func) { func(http::HttpHandlerGroups(routes_)); }

  /// Gets routes object (for tests / introspection).
  http::HttpRoutes& GetRoutes() { return routes_; }

 protected:
  /// Dispatch request to handler (wrapper around routes_.Handle).
  Status Dispatch(const std::string& path, http::HandlerBase* handler, ServerContextPtr& context, http::RequestPtr& req,
                  http::Response& rsp);

 private:
  /// Internal handler calling logic copied/adapted from HttpService.
  void Handle(const std::string& path, http::HandlerBase* handler, ServerContextPtr& context, http::RequestPtr& req,
              http::Response& rsp, STransportRspMsg** send);

  static void HandleError(ServerContextPtr& context, http::RequestPtr& req, http::Response& rsp, const Status& status);

  void CheckTimeout(const ServerContextPtr& context);

 protected:
  http::Routes routes_;
};

}  // namespace trpc

