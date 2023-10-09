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

#include <memory>

#include "trpc/admin/admin_handler.h"

namespace trpc::admin {

/// @brief Handles the request for getting version of framework, then replies the version of tRPC.
class VersionHandler : public AdminHandlerBase {
 public:
  VersionHandler() { description_ = "[GET /version]    get trpc version"; }
  ~VersionHandler() override = default;

  /// @brief Handles the HTTP request for getting version of tRPC.
  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

}  // namespace trpc::admin
