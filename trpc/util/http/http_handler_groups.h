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

#include <typeindex>
#include <unordered_map>

#include "trpc/util/http/http_handler.h"
#include "trpc/util/http/routes.h"
#include "trpc/util/http/stream/http_stream_handler.h"

#define TRPC_HTTP_ROUTE_ENDPOINT(endpoint, http_method)                                                             \
  template <typename T>                                                                                             \
  HttpHandlerGroups& endpoint(std::string_view path = "") {                                                         \
    return Handle<T, http_method>(path);                                                                            \
  }                                                                                                                 \
  template <typename T = trpc::http::HttpHandler>                                                                   \
  HttpHandlerGroups& endpoint(trpc::http::HttpHandlerGroups::HttpHandlerFunc http_handler) {                        \
    return Handle<T, http_method>("", std::move(http_handler));                                                     \
  }                                                                                                                 \
  template <typename T = trpc::http::HttpHandler>                                                                   \
  HttpHandlerGroups& endpoint(std::string_view path, trpc::http::HttpHandlerGroups::HttpHandlerFunc http_handler) { \
    return Handle<T, http_method>(path, std::move(http_handler));                                                   \
  }                                                                                                                 \
  HttpHandlerGroups& endpoint(std::shared_ptr<trpc::http::HandlerBase> handler) {                                   \
    return Handle<http_method>("", std::move(handler));                                                             \
  }                                                                                                                 \
  HttpHandlerGroups& endpoint(std::string_view path, std::shared_ptr<trpc::http::HandlerBase> handler) {            \
    return Handle<http_method>(path, std::move(handler));                                                           \
  }

#define TRPC_HTTP_ROUTE_HANDLER_ENDPOINT(handler_endpoint)                                                    \
  template <typename T>                                                                                       \
  void handler_endpoint(std::string_view path = "") {                                                         \
    route_->handler_endpoint<T>(path);                                                                        \
  }                                                                                                           \
  template <typename T = trpc::http::HttpHandler>                                                             \
  void handler_endpoint(trpc::http::HttpHandlerGroups::HttpHandlerFunc http_handler) {                        \
    route_->handler_endpoint<T>("", std::move(http_handler));                                                 \
  }                                                                                                           \
  template <typename T = trpc::http::HttpHandler>                                                             \
  void handler_endpoint(std::string_view path, trpc::http::HttpHandlerGroups::HttpHandlerFunc http_handler) { \
    route_->handler_endpoint<T>(path, std::move(http_handler));                                               \
  }                                                                                                           \
  void handler_endpoint(std::shared_ptr<trpc::http::HandlerBase> handler) {                                   \
    route_->handler_endpoint("", std::move(handler));                                                         \
  }                                                                                                           \
  void handler_endpoint(std::string_view path, std::shared_ptr<trpc::http::HandlerBase> handler) {            \
    route_->handler_endpoint(path, std::move(handler));                                                       \
  }

/// @brief Returns a route handler.
/// @param route is a function to initialize an HTTP handler.
#define TRPC_HTTP_ROUTE_HANDLER(route)                                        \
  [&]() {                                                                     \
    struct __trpc_http_this__ : trpc::http::HttpHandlerGroups::RouteHandler { \
      void Apply() override { [&] route(); }                                  \
    };                                                                        \
    return std::make_unique<__trpc_http_this__>();                            \
  }()
#define TRPC_HTTP_ROUTE_HANDLER_ARGS(args, route...)                               \
  [&](const auto& __trpc_http_args__) {                                            \
    struct __trpc_http_this__ : trpc::http::HttpHandlerGroups::RouteHandler {      \
      decltype(__trpc_http_args__) __trpc_http_this_args__;                        \
      explicit __trpc_http_this__(decltype(__trpc_http_args__) __trpc_this_args__) \
          : __trpc_http_this_args__(std::move(__trpc_this_args__)) {}              \
      void Apply() override { std::apply([&] route, __trpc_http_this_args__); }    \
    };                                                                             \
    return std::make_unique<__trpc_http_this__>(__trpc_http_args__);               \
  }(std::make_tuple args)
#define TRPC_HTTP_HANDLER(handler, func)                                                                     \
  [&, __trpc_http_handler__ = handler](const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req, \
                                       trpc::http::Response* rsp) {                                          \
    return (__trpc_http_handler__.operator->()->*&func)(ctx, req, rsp);                                      \
  }
#define TRPC_HTTP_STREAM_HANDLER(handler, func) \
  std::make_shared<trpc::http::StreamHandlerFuncWrapper<decltype(handler)>>(handler, &func)

namespace trpc::http {

/// @brief HTTP grouped routing handler.
class HttpHandlerGroups {
 public:
  using HttpHandlerFunc =
      std::function<trpc::Status(const trpc::ServerContextPtr&, const trpc::http::RequestPtr&, trpc::http::Response*)>;

  /// @private For internal use purpose only.
  class RouteHandler {
   public:
    virtual ~RouteHandler() = default;

    void Path(std::string_view path, std::unique_ptr<RouteHandler> route_handler) {
      route_->Path(path, std::move(route_handler));
    }

    TRPC_HTTP_ROUTE_HANDLER_ENDPOINT(Get)
    TRPC_HTTP_ROUTE_HANDLER_ENDPOINT(Head)
    TRPC_HTTP_ROUTE_HANDLER_ENDPOINT(Post)
    TRPC_HTTP_ROUTE_HANDLER_ENDPOINT(Put)
    TRPC_HTTP_ROUTE_HANDLER_ENDPOINT(Delete)
    TRPC_HTTP_ROUTE_HANDLER_ENDPOINT(Options)
    TRPC_HTTP_ROUTE_HANDLER_ENDPOINT(Patch)

    void SetHttpRoute(std::unique_ptr<HttpHandlerGroups> route) { route_ = std::move(route); }

    virtual void Apply() = 0;

   private:
    std::unique_ptr<HttpHandlerGroups> route_;
  };

  explicit HttpHandlerGroups(Routes& routes)
      : routes_(routes),
        handlers_(std::make_shared<std::unordered_map<std::type_index, std::shared_ptr<HandlerBase>>>()) {}

  HttpHandlerGroups(HttpHandlerGroups& route, std::string basic_path)
      : routes_(route.routes_), handlers_(route.handlers_), basic_path_(std::move(basic_path)) {}

  /// @brief Adds sub-path route handler.
  HttpHandlerGroups& Path(std::string_view path, std::unique_ptr<RouteHandler> route_handler) {
    route_handler->SetHttpRoute(std::make_unique<HttpHandlerGroups>(*this, NormalizePath(path)));
    route_handler->Apply();
    return *this;
  }

  TRPC_HTTP_ROUTE_ENDPOINT(Get, MethodType::GET)
  TRPC_HTTP_ROUTE_ENDPOINT(Head, MethodType::HEAD)
  TRPC_HTTP_ROUTE_ENDPOINT(Post, MethodType::POST)
  TRPC_HTTP_ROUTE_ENDPOINT(Put, MethodType::PUT)
  TRPC_HTTP_ROUTE_ENDPOINT(Delete, MethodType::DELETE)
  TRPC_HTTP_ROUTE_ENDPOINT(Options, MethodType::OPTIONS)
  TRPC_HTTP_ROUTE_ENDPOINT(Patch, MethodType::PATCH)

 private:
  template <typename T, MethodType Method>
  HttpHandlerGroups& Handle(std::string_view path) {
    auto iter = handlers_->find(typeid(T));
    if (iter == handlers_->end()) {
      iter = handlers_->insert({typeid(T), std::make_shared<T>()}).first;
    }
    return Handle<Method>(path, iter->second);
  }

  template <typename T, MethodType Method>
  HttpHandlerGroups& Handle(std::string_view path, HttpHandlerFunc http_handler) {
    class Handler : public T {
     public:
      Handler(MethodType method, HttpHandlerFunc handler) : method_(method), handler_(std::move(handler)) {}

      Status Handle(const trpc::ServerContextPtr& ctx, const trpc::http::RequestPtr& req,
                    trpc::http::Response* rsp) override {
        Status status;
        if (req->GetMethodType() == method_) {
          status = handler_(ctx, req, rsp);
        }
        return status;
      }

     private:
      using T::Handle;

     private:
      MethodType method_;
      HttpHandlerFunc handler_;
    };
    auto handler = std::make_shared<Handler>(Method, std::move(http_handler));
    return Handle<Method>(path, std::move(handler));
  }

  template <MethodType Method>
  HttpHandlerGroups& Handle(std::string_view path, std::shared_ptr<HandlerBase> handler) {
    routes_.Add(Method, HttpPath(NormalizePath(path)), std::move(handler));
    return *this;
  }

  std::string NormalizePath(std::string_view path) const {
    if (path.empty()) {
      return basic_path_;
    }
    std::string full_path = basic_path_;
    if (path[0] != '/') {
      full_path.push_back('/');
    }
    if (path[path.length() - 1] == '/') {
      path.remove_suffix(1);
    }
    return full_path.append(path);
  }

  static trpc::http::Path HttpPath(std::string path) {
    if (path.find('<') != std::string::npos && path.find('>') != std::string::npos) {
      path.insert(0, "<ph(").append(")>");
    }
    return trpc::http::Path(std::move(path));
  }

 private:
  Routes& routes_;
  std::shared_ptr<std::unordered_map<std::type_index, std::shared_ptr<HandlerBase>>> handlers_;
  std::string basic_path_;
};

}  // namespace trpc::http

#undef TRPC_HTTP_ROUTE_ENDPOINT
#undef TRPC_HTTP_ROUTE_HANDLER_ENDPOINT
