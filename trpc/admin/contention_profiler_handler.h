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
#include <string>

#include "trpc/admin/admin_handler.h"

namespace trpc::admin {

/// @brief Handles the request for getting contention profile.
class WebContentionProfilerHandler : public AdminHandlerBase {
 public:
  WebContentionProfilerHandler() { description_ = "[GET /cmdsweb/profile/contention] start contention profiling"; }

  ~WebContentionProfilerHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  const char* SetUpdateEvents();
};

/// @brief Handles the request for getting contention profile.
class WebContentionProfilerDrawHandler : public AdminHandlerBase {
 public:
  WebContentionProfilerDrawHandler() {
    description_ = "[POST /cmdsweb/profile/contention_draw] start contention profiling drawing";
  }

  ~WebContentionProfilerDrawHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

}  // namespace trpc::admin
