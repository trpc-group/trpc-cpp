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

#include <map>
#include <memory>
#include <string>

#include "trpc/admin/admin_handler.h"

namespace trpc::admin {

/// @brief Handles the request for getting CPU profile.
class CpuProfilerHandler : public AdminHandlerBase {
 public:
  CpuProfilerHandler() { description_ = "[POST /cmds/profile/cpu?enable=y/n] start/stop cpu profiling"; }

  ~CpuProfilerHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

/// @brief Handles the request for getting CPU profile.
class WebCpuProfilerHandler : public AdminHandlerBase {
 public:
  WebCpuProfilerHandler() { description_ = "[POST /cmdsweb/profile/cpu] start cpu profiling"; }

  ~WebCpuProfilerHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  const char* SetUpdateEvents();
};

/// @brief Handles the request for getting CPU profile.
class WebCpuProfilerDrawHandler : public AdminHandlerBase {
 public:
  WebCpuProfilerDrawHandler() { description_ = "[POST /cmdsweb/profile/cpu_draw] start cpu profiling drawing"; }

  ~WebCpuProfilerDrawHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

}  // namespace trpc::admin
