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

#include "spdlog/pattern_formatter.h"

#include "trpc/common/plugin.h"
#include "trpc/util/log/log.h"

namespace trpc {

class Logging : public Plugin {
 public:
  /// @brief Gets the logger name.
  /// @return Under which logger the remote logging plugin is mounted.
  virtual const std::string& LoggerName() const = 0;

  /// @brief Gets the plugin type
  /// @return plugin type
  PluginType Type() const override { return PluginType::kLogging; }

  /// @brief  Output functions for multithreading safety.
  /// @param  level         Log level
  /// @param  filename_in   which source file this log comes from
  /// @param  line_in       which line in the source file this log comes from
  /// @param  funcname_in   which function in the source file this log comes from
  /// @param  msg           Log message
  virtual void Log(const Log::Level level, const char* filename_in, int line_in, const char* funcname_in,
                   std::string_view msg, const std::unordered_map<uint32_t, std::any>& extend_fields_msg) = 0;

  /// @brief Get the pointer of sink instance of spdlog
  virtual spdlog::sink_ptr SpdSink() const { return nullptr; }
};

using LoggingPtr = RefPtr<Logging>;

}  // namespace trpc
