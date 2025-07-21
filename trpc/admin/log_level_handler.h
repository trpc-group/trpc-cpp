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

/// @brief Handles the request for setting log level, then prints log level settings.
/// It also supports PUT method to update log level settings.
class LogLevelHandler : public AdminHandlerBase {
 public:
  explicit LogLevelHandler(bool update_flag = false) : update_flag_(update_flag) {
    description_ =
        "[GET  /loglevel?instance={INSTANCE}] get trpc log level, "
        "[PUT /loglevel?instance={INSTANCE}] set trpc log level. "
        "(content eg. value=WARNING)";
  }

  ~LogLevelHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  bool update_flag_;
};

/// @brief Handles the request which comes from web browser for setting log.
class WebLogLevelHandler : public AdminHandlerBase {
 public:
  WebLogLevelHandler() { description_ = "[GET  /cmdsweb/logs get trpc log config"; }

  ~WebLogLevelHandler() override = default;

  /// @brief Handles the HTTP request for setting log.
  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  // @brief Generates a piece of javascript code for handling log setting click event.
  const char* SetUpdateEvents();
  // @brief Generates a piece of html code for log level options.
  void GetLogLevelOption(std::string* html);
  // @brief Generates a piece of html-table code for log settings.
  void PrintLogConfig(std::string* html, LogPtr log, const std::string& instance);
};

}  // namespace trpc::admin
