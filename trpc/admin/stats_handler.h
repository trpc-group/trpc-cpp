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

#include <memory>
#include <string>

#include "trpc/admin/admin_handler.h"

namespace trpc::admin {

/// @brief Handles the request for getting status of connections, requests, then replies the count of connections,
/// requests.
class StatsHandler : public AdminHandlerBase {
 public:
  StatsHandler() { description_ = "[GET /cmds/stats] get server stats(connections, requests)"; }

  ~StatsHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

/// @brief Handles the request for getting status of connections, requests, then replies the count of connections,
/// requests.
class WebStatsHandler : public AdminHandlerBase {
 public:
  WebStatsHandler() { description_ = "[GET /cmdsweb/stats] get server stats(connections,rquests)"; }

  ~WebStatsHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

/// @brief Handles the request for getting inner status (vars), then replies the count of the inner vars,
class VarHandler : public AdminHandlerBase {
 public:
  explicit VarHandler(std::string prefix);

  ~VarHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override {}

  trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, http::HttpRequestPtr req,
                      http::HttpResponse* reply) override;

 private:
  std::string uri_prefix_;
};

#ifdef TRPC_BUILD_INCLUDE_RPCZ
/// @brief Handles the request for getting rpcz, then replies the content of the rpcz.
class RpczHandler : public AdminHandlerBase {
 public:
  explicit RpczHandler(std::string&& prefix);

  ~RpczHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override {}

  trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, http::HttpRequestPtr req,
                      http::HttpResponse* reply) override;

 private:
  std::string url_prefix_;
};
#endif

}  // namespace trpc::admin
