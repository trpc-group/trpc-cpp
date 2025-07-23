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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "trpc/util/http/handler.h"
#include "trpc/util/http/match_rule.h"
#include "trpc/util/http/method.h"
#include "trpc/util/http/parameter.h"
#include "trpc/util/http/path.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/http/util.h"

namespace trpc {

class ServerContext;
using ServerContextPtr = RefPtr<ServerContext>;

namespace http {
// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/include/seastar/http/routes.hh.

/// @brief Dispatches requests based on URL. Performs extract matching first (Leading slash is permitted),
/// and if it fails, matches one by one according to the order of the routing rules (Insertion order).
class Routes {
 public:
  /// @brief Adds a matching rule which is only used when the extract matching rule is not found, and is searched
  /// in the order of insertion.
  /// @param rule is a matching rule to add.
  /// @param type is the type of HTTP method.
  /// @return Returns a reference to the itself.
  Routes& Add(MatchRule* rule, MethodType type) { return Add(std::shared_ptr<MatchRule>(rule), type); }
  Routes& Add(std::shared_ptr<MatchRule> rule, MethodType type);

  /// @brief Add a matching rule to a handler.
  /// e.g. routes.Add(GET, trpc::http::Path("/api").Remainder("path"), handler)
  /// @param type is the type of HTTP method.
  /// @param path is the path of request URI.
  /// @param handler is the handler pointer.
  /// @return Returns a reference to the itself of 'Routes' object.
  Routes& Add(MethodType type, const Path& path, std::shared_ptr<HandlerBase> handler);

  /// @brief Deprecated: use "Add(MethodType, const Path&, std::shared_ptr<HandlerBase>)" instead.
  /// @note  Users need to manually manage the handler object, handler is not freed in 'Routes''s destructor.
  [[deprecated("use shared_ptr interface for automatic object lifetime management")]] Routes& Add(
      MethodType type, const Path& path, HandlerBase* handler) {
    return Add(type, path, std::shared_ptr<HandlerBase>(handler, [](auto*) {}));
  }

  /// @brief Adds a matching rule as an exact matching.
  /// @param type is the type of HTTP method.
  /// @param path is the path of request URI to match which starts with a slash ("/").
  /// @param handler is the handler pointer
  /// @return Returns a reference to the itself of 'Routes' object.
  Routes& Put(MethodType type, const std::string& path, std::shared_ptr<HandlerBase> handler) {
    exact_rules_[type][path] = std::move(handler);
    return *this;
  }

  /// @brief Deprecated: use "Put(MethodType, const Path&, std::shared_ptr<HandlerBase>)" instead.
  /// @note  Users need to manually manage the handler object, handler is not freed in 'Routes''s destructor.
  [[deprecated("use shared_ptr interface for automatic object lifetime management")]] Routes& Put(
      MethodType type, const std::string& path, HandlerBase* handler) {
    return Put(type, path, std::shared_ptr<HandlerBase>(handler, [](auto*) {}));
  }

  /// @brief Gets an exact matching rule.
  /// @param type is the type of HTTP method.
  /// @param path is the path of request URI to match.
  /// @return  Returns an handler if a matched rule exists, nullptr otherwise.
  HandlerBase* GetExactMatch(MethodType type, const std::string& path) {
    auto iter = exact_rules_[type].find(path);
    return iter == exact_rules_[type].end() ? nullptr : iter->second.get();
  }

  /// @brief Gets an exact matching rule.
  /// @param type is the type of HTTP method.
  /// @param path is the path of request URI to match.
  /// @param params is the path parameters, it will be filled if an extract matching rule fails to match and
  /// other parameter matching rule is matched successfully.
  /// @return  Returns an handler if a matched rule exists, nullptr otherwise.
  HandlerBase* GetHandler(MethodType type, const std::string& path, Parameters& params);

  /// @brief Gets an exact matching rule.
  /// @param path is the path of request URI to match.
  /// @param req is the HTTP request, it's path parameters will be filled if an extract matching rule fails to match
  /// and other parameter matching rule is matched successfully.
  /// @return  Returns an handler if a matched rule exists, nullptr otherwise.
  HandlerBase* GetHandler(const std::string& path, RequestPtr& req);

  /// @brief Serving the HTTP request.
  /// The general handler calls this method with the request parameter.
  /// The method takes the header from the request and find the right handler.
  /// @param path is the path of request URI.
  /// @param req is the HTTP request.
  /// @param rsp is the HTTP response.
  /// @return Returns the status of request processing.
  trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, RequestPtr& req, Response& rsp) {
    std::string normalized_path = trpc::http::NormalizeUrl(path);
    HandlerBase* handler = GetHandler(normalized_path, req);
    return Handle(normalized_path, handler, std::move(context), req, rsp);
  }
  trpc::Status Handle(const std::string& path, HandlerBase* handler, trpc::ServerContextPtr context, RequestPtr& req,
                      Response& rsp);

  /// @brief Converts an exception to HTTP response by JSON (default).
  /// @param eptr is a pointer of exception.
  /// @param rsp is the HTTP response.
  static void ExceptionReply(std::exception_ptr eptr, Response* rsp);

 private:
  std::unordered_map<std::string, std::shared_ptr<HandlerBase>> exact_rules_[MethodType::UNKNOWN+1];
  std::vector<std::shared_ptr<MatchRule>> rules_[MethodType::UNKNOWN+1];
};

using HttpRoutes = Routes;
// End of source codes that are from seastar.

}  // namespace http

}  // namespace trpc
