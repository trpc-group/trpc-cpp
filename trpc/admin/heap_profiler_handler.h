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

/// @brief Handles the request for getting heap profile.
class HeapProfilerHandler : public AdminHandlerBase {
 public:
  HeapProfilerHandler() { description_ = "[POST /cmds/profile/heap?enable=y/n] start/stop heap profiling"; }

  ~HeapProfilerHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

/// @brief Handles the request for getting heap profile.
class WebHeapProfilerHandler : public AdminHandlerBase {
 public:
  WebHeapProfilerHandler() { description_ = "[GET /cmdsweb/profile/heap] start heap profiling"; }

  ~WebHeapProfilerHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  const char* SetUpdateEvents();
};

/// @brief Handles the request for getting heap profile.
class WebHeapProfilerDrawHandler : public AdminHandlerBase {
 public:
  WebHeapProfilerDrawHandler() { description_ = "[POST /cmdsweb/profile/heap_draw] start heap profiling drawing"; }

  ~WebHeapProfilerDrawHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

}  // namespace trpc::admin
