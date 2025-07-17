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

/// @brief Handles the request for getting status of system resource, then replies the content of CPU, memory, IO.
class WebSysVarsHandler : public AdminHandlerBase {
 public:
  WebSysVarsHandler() { description_ = "[GET /cmdsweb/sysvars] get server system vars(cpu, memory, io, disk...)"; }

  ~WebSysVarsHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

  static void PrintProcRusage(std::string* html, bool force);

  static void PrintProcIoAndMem(std::string* html, bool force);

  static void PrintSysVarsData(std::string* html, bool force = false);

  static void SetFlotVar(std::string* html, const std::string& var_name, const std::string& value);

  static void SetNonFlotVar(std::string* html, const std::string& var_name, const std::string& value);
};

/// @brief Handles the request for getting status of system resource, then replies the content of CPU, memory, IO.
class WebSeriesVarHandler : public AdminHandlerBase {
 public:
  WebSeriesVarHandler() {
    description_ = "[GET /cmdsweb/seriesvar] get series data for sysvars(cpu, memory, io, disk...)";
  }

  ~WebSeriesVarHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

}  // namespace trpc::admin
