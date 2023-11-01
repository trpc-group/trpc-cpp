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

#include <unistd.h>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include "trpc/admin/admin_handler.h"
#include "trpc/server/http_service.h"

namespace trpc {

/// @brief Serving admin request. It also provides the interface to register admin handlers defined by user.
class AdminService : public HttpService {
 public:
  AdminService();
  ~AdminService() override = default;

  /// @brief Builds options for admin service.
  static bool BuildServiceAdapterOption(ServiceAdapterOption& option);

  /// @brief Sets request route for admin commands.
  /// @note User need to manually manage the handler object, handler is not freed in AdminService destructor
  void RegisterCmd(http::OperationType type, const std::string url, AdminHandlerBase* handler);

  /// @brief Sets request route for admin commands.
  void RegisterCmd(http::OperationType type, const std::string& path,
                   const std::shared_ptr<http::HandlerBase>& handler);

 private:
  void ListCmds(http::HttpRequestPtr req, rapidjson::Value& result, rapidjson::Document::AllocatorType& alloc);

  void StartSysvarsTask();

 private:
  std::set<std::string> unrepeatable_commands_;
};

using AdminServicePtr = std::shared_ptr<AdminService>;

}  // namespace trpc
