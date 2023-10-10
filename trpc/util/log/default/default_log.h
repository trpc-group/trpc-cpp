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

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "spdlog/async.h"
#include "spdlog/spdlog.h"

#include "trpc/common/config/default_log_conf.h"
#include "trpc/log/logging.h"
#include "trpc/util/log/log.h"

namespace trpc {

/// @brief The "default" log plugin provided by trpc-cpp, based on spdlog, supports multi-sink extension.
class DefaultLog : public Log {
 public:
  /// @brief logger instance
  struct Logger {
    /// logger instance configuration
    DefaultLogConfig::LoggerInstance config;

    /// Same reason as above.
    std::shared_ptr<spdlog::logger> logger;

    /// Because the underlying spdlog thread_pool is a shared_ptr, the definition is consistent with it here.
    std::shared_ptr<spdlog::details::thread_pool> thread_pool;

    /// Set of raw_sinks
    std::vector<LoggingPtr> raw_sinks;
  };

  /// @brief We can use both interfaces when you need to create
  /// or stop threads in your plugin's internal implementation.
  void Start() override;
  void Stop() override;

  /// @brief Initialize the "default" log plugin
  /// @return 0 on success, otherwise on fail.
  int Init();

  /// @brief As opposed to initialization, resources need to be released here.
  int Destroy() override;

  /// @brief Register a new logger instance.
  /// @param  instance_name Log instance name
  /// @param  logger        Log instance
  void RegisterInstance(const char* instance_name, Logger logger);

  /// @brief Get the logger instance with the specified name.
  /// @param instance_name The name of the logger instance to retrieve.
  /// @return Logger* A pointer to the logger instance, or nullptr if the instance does not exist.
  Logger* GetLoggerInstance(const std::string& instance_name);

  /// @brief Determine whether the log level of the current instance meets the requirements for printing this log.
  /// @param  instance_name Log instance name
  /// @param  level         Log instance level
  /// @return true/false
  bool ShouldLog(const char* instance_name, Level level) const override;

  /// @brief  Output log to a sink instance.
  void LogIt(const char* instance_name, Level level, const char* filename_in, int line_in, const char* funcname_in,
             std::string_view msg, const std::unordered_map<uint32_t, std::any>& filter_data = {}) const override;

  /// @brief Gets the priority of the current log instance
  /// @param  instance_name Log instance name
  /// @return std::pair<Level, bool>  Get the log level configured for the instance
  std::pair<Level, bool> GetLevel(const char* instance_name) const override;

  /// @brief Sets the priority of the current log instance.
  /// @param  instance_name Log instance name
  /// @param  level         Log instance level
  /// @return std::pair<Level, bool>  Set the log level configured for the instance.
  std::pair<Level, bool> SetLevel(const char* instance_name, Level level) override;

  /// @brief Registering the LocalFile sink to spdlog's Logger
  /// @param logger_name The instance name to init
  /// @return bool success/failure
  /// @private For internal use purpose only.
  template <typename Sink, typename SinkConfig>
  bool InitSink(const char* logger_name) {
    auto& instance = instances_[logger_name];
    auto& conf = instance.config;
    // Get local_file configuration
    SinkConfig sink_config;
    // configuration does not exist and returns true
    if (!GetLoggerConfig<SinkConfig>(conf.name, sink_config)) return true;
    auto sink = MakeRefCounted<Sink>();
    sink->Init(sink_config);
    // Add the new sink to the logger's sinks
    instance.logger->sinks().push_back(sink->SpdSink());

    return true;
  }

  /// @brief Registering the remote sink to spdlog's Logger
  /// @param logger_name The logger_name to init
  /// @return bool success/failure
  /// @private For internal use purpose only.
  bool InitRawSink(const char* logger_name);

  /// @brief Users need to call this interface to complete the registration of
  /// the remote plugin before the initialization of the trpc-cpp plugin
  /// @param raw_sink The LoggingPtr object associated with the raw sink
  /// @return bool
  bool RegisterRawSink(const LoggingPtr& raw_sink);

  /// @brief Retrieves the raw sink with the specified name.
  /// @param raw_sink_name The name of the raw sink to retrieve.
  /// @param logger_name The name of the logger instance to search in (optional).
  /// @return A reference to the LoggingPtr object associated with the raw sink,
  ///         or an empty LoggingPtr if no matching raw sink is found.
  LoggingPtr& GetRawSink(const char* raw_sink_name, const char* logger_name = nullptr);

 private:
  /// @brief The log level of trpc translates to the log level of spdlog
  /// @param Log level of trpc
  /// @param spdlog::level::level_enum The log level of spdlog
  static spdlog::level::level_enum SpdLevel(Level level) {
    return static_cast<spdlog::level::level_enum>(static_cast<int>(level));
  }

  // Create an output logger for spdlog
  bool createSpdLogger(const char* logger_name);

 private:
  // Default queue length
  static constexpr size_t kThreadPoolQueueSize = 100000;

  // Initialization flags
  bool inited_{false};

  // Collection of log instances
  std::unordered_map<std::string, Logger> instances_;
};

using DefaultLogPtr = RefPtr<DefaultLog>;

}  // namespace trpc
