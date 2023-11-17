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

#include "trpc/util/log/default/default_log.h"

#include <string_view>

#include "trpc/common/config/default_value.h"
#include "trpc/common/config/local_file_sink_conf.h"
#include "trpc/common/config/local_file_sink_conf_parser.h"
#include "trpc/common/config/stdout_sink_conf.h"
#include "trpc/common/config/stdout_sink_conf_parser.h"
#include "trpc/util/log/default/sinks/local_file/local_file_sink.h"
#include "trpc/util/log/default/sinks/stdout/stdout_sink.h"

namespace trpc {

void DefaultLog::Start() { spdlog::flush_every(std::chrono::milliseconds{50}); }

void DefaultLog::Stop() {
  for (auto& instance : instances_) {
    for (auto& sink : instance.second.raw_sinks) {
      sink->Stop();
    }
    if (instance.second.logger) {
      instance.second.logger->flush();
    }
  }
}

bool DefaultLog::ShouldLog(const char* instance_name, Level level) const {
  if (!initted_) {
    std::cerr << "DefaultLog not initted" << std::endl;
    return false;
  }

  auto iter = instances_.find(instance_name);
  if (iter == instances_.end()) {
    std::cerr << "DefaultLog instance " << instance_name << " does not exit" << std::endl;
    return false;
  }
  auto& instance = iter->second;
  return level >= instance.config.min_level;
}

bool DefaultLog::ShouldLog(Level level) const {
  if (!initted_) {
    std::cerr << "DefaultLog not initted" << std::endl;
    return false;
  }
  if (!initted_trpc_logger_instance_) {
    std::cout << "not inited!" << std::endl;
    return false;
  }
  return level >= trpc_logger_instance_.config.min_level;
}

void DefaultLog::LogIt(const char* instance_name, Level level, const char* filename_in, int line_in,
                       const char* funcname_in, std::string_view msg,
                       const std::unordered_map<uint32_t, std::any>& filter_data) const {
  if (!initted_) {
    std::cerr << "DefaultLog not initted" << std::endl;
    return;
  }

  const DefaultLog::Logger* instance = nullptr;
  // It is preferred if it is the output of the tRPC-Cpp framework log
  if (!strcmp(instance_name, kTrpcLogCacheStringDefault)) {
    if (initted_trpc_logger_instance_ == false) {
      std::cerr << "DefaultLog instance: " << kTrpcLogCacheStringDefault << " does not exit" << std::endl;
      return;
    }
    instance = &trpc_logger_instance_;
  } else {
    auto iter = instances_.find(instance_name);
    if (iter == instances_.end()) {
      std::cerr << "DefaultLog instance: " << instance_name << " does not exit" << std::endl;
      return;
    }
    instance = &iter->second;
  }

  if (instance->logger) {
    instance->logger->log(spdlog::source_loc{filename_in, line_in, funcname_in}, SpdLevel(level), msg);
  }

  // Output to a remote plugin (if available)
  for (const auto& sink : instance->raw_sinks) {
    sink->Log(level, filename_in, line_in, funcname_in, msg, filter_data);
  }
}

std::pair<Log::Level, bool> DefaultLog::GetLevel(const char* instance_name) const {
  if (!initted_) {
    std::cerr << "DefaultLog not initted" << std::endl;
    return std::make_pair(Level::info, false);
  }

  auto iter = instances_.find(instance_name);
  if (iter == instances_.end()) {
    std::cerr << "DefaultLog instance " << instance_name << " does not exit" << std::endl;
    return std::make_pair(Level::info, false);
  }
  auto& instance = iter->second;

  return std::make_pair(static_cast<Log::Level>(instance.config.min_level), true);
}

std::pair<Log::Level, bool> DefaultLog::SetLevel(const char* instance_name, Level level) {
  if (!initted_) {
    std::cerr << "DefaultLog not initted" << std::endl;
    return std::make_pair(Level::info, false);
  }

  auto iter = instances_.find(instance_name);
  if (iter == instances_.end()) {
    std::cerr << "DefaultLog instance " << instance_name << " does not exit" << std::endl;
    return std::make_pair(Level::info, false);
  }
  auto& instance = iter->second;

  auto old = static_cast<Log::Level>(instance.config.min_level);
  instance.config.min_level = static_cast<unsigned int>(level);
  if (!strcmp(instance_name, kTrpcLogCacheStringDefault)) {
    trpc_logger_instance_.config.min_level = static_cast<unsigned int>(level);
  }
  return std::make_pair(old, true);
}

int DefaultLog::Init() {
  if (initted_) {
    std::cerr << "DefaultLog already init!" << std::endl;
    return -1;
  }

  // Get all logger instances configured for the "default" plugin
  DefaultLogConfig config;
  if (!GetDefaultLogConfig(config)) {
    return -1;
  }

  // "config.instances" stands for the logger instance,
  // users can configure multiple loggers to separate business logs from framework logs.
  for (const auto& conf : config.instances) {
    // Register the Logger instance
    RegisterInstance(conf.name.c_str(), Logger{conf});

    // Create the spdlog logger
    if (!CreateSpdLogger(conf.name.c_str())) {
      std::cerr << "create SpdLogger failed! name: " << conf.name.c_str() << std::endl;
      return -1;
    }

    // Init sinks into the spdlog's Logger if exist
    if (!InitSink<LocalFileSink, LocalFileSinkConfig>(conf.name.c_str(), "local_file")) {
      return -1;
    }
    if (!InitSink<StdoutSink, StdoutSinkConfig>(conf.name.c_str(), "stdout")) {
      return -1;
    }
  }
  initted_ = true;
  return 0;
}

bool DefaultLog::RegisterRawSink(const LoggingPtr& raw_sink) {
  if (raw_sink == nullptr) {
    std::cerr << "raw_sink is nullptr!" << std::endl;
    return false;
  }
  const std::string& logger_name = raw_sink->LoggerName();
  auto& instance = instances_[logger_name];
  auto spd_sink = raw_sink->SpdSink();
  if (spd_sink) {
    instance.logger->sinks().push_back(spd_sink);
  } else {
    instance.raw_sinks.push_back(raw_sink);
  }
  return true;
}

LoggingPtr& DefaultLog::GetRawSink(const char* raw_sink_name, const char* logger_name) {
  static LoggingPtr empty_logging_ptr;

  for (auto& instance_pair : instances_) {
    // If a logger_name is provided, skip comparisons with other instances
    if (logger_name && instance_pair.first != logger_name) {
      continue;
    }

    for (auto& raw_sink : instance_pair.second.raw_sinks) {
      if (raw_sink->Name() == raw_sink_name) {
        return raw_sink;
      }
    }

    // If a logger_name is provided and its associated instance has been checked, break the loop early
    if (logger_name) {
      break;
    }
  }

  return empty_logging_ptr;
}

void DefaultLog::RegisterInstance(const char* instance_name, DefaultLog::Logger logger) {
  instances_[instance_name] = std::move(logger);
}

DefaultLog::Logger* DefaultLog::GetLoggerInstance(const std::string& instance_name) {
  auto it = instances_.find(instance_name);
  if (it != instances_.end()) {
    return &(it->second);
  } else {
    std::cerr << "instance_name not found!" << std::endl;
    return nullptr;
  }
}

bool DefaultLog::CreateSpdLogger(const char* logger_name) {
  auto& instance = instances_[logger_name];
  auto& conf = instance.config;

  if (conf.mode > 3 || conf.mode < 1) {
    std::cerr << "mode " << conf.mode << " is invalid" << std::endl;
    return false;
  }

  if (conf.mode == 1) {
    instance.logger = std::make_shared<spdlog::logger>(logger_name);
  } else {
    auto policy = conf.mode == 2 ? spdlog::async_overflow_policy::block : spdlog::async_overflow_policy::overrun_oldest;
    // Configure spdlog thread pool
    instance.thread_pool = std::make_shared<spdlog::details::thread_pool>(kThreadPoolQueueSize, 1);
    instance.logger =
        std::make_shared<spdlog::async_logger>(logger_name, spdlog::sinks_init_list{}, instance.thread_pool, policy);
  }

  // Register the logger to flush the logs regularly.
  instance.logger->flush_on(spdlog::level::critical);
  instance.logger->set_level(spdlog::level::trace);
  spdlog::register_logger(instance.logger);

  return true;
}

int DefaultLog::Destroy() {
  if (!initted_) {
    std::cerr << "DefaultLog not initted" << std::endl;
    exit(-1);
  }

  initted_ = false;
  for (auto& instance : instances_) {
    for (auto& sink : instance.second.raw_sinks) {
      sink->Destroy();
    }
  }
  spdlog::shutdown();
  return 0;
}

}  // namespace trpc
