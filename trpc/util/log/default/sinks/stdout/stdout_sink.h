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
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/version.h"

#include "trpc/common/config/stdout_sink_conf.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class StdoutSink : public RefCounted<StdoutSink> {
  class Sink : public spdlog::sinks::sink {
   public:
    explicit Sink(const StdoutSinkConfig& config) {
      stdout_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      stderr_sink_ = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
      stderr_level_ = config.stderr_level;
      std::string eol = config.eol ? spdlog::details::os::default_eol : "";
      auto formatter = config.format.empty() ? std::make_unique<spdlog::pattern_formatter>(
                                                   spdlog::pattern_time_type::local, std::move(eol))
                                             : std::make_unique<spdlog::pattern_formatter>(
                                                   config.format, spdlog::pattern_time_type::local, std::move(eol));
      stdout_sink_->set_formatter(formatter->clone());
      stderr_sink_->set_formatter(std::move(formatter));
    }

    void log(const spdlog::details::log_msg& msg) override {
      if (msg.level < static_cast<int>(stderr_level_)) {
        stdout_sink_->log(msg);
      } else {
        stderr_sink_->log(msg);
      }
    }

    void flush() override {
      stdout_sink_->flush();
      stderr_sink_->flush();
    }

    void set_pattern(const std::string& pattern) override {
      stdout_sink_->set_pattern(pattern);
      stderr_sink_->set_pattern(pattern);
    }

    void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override {
      stdout_sink_->set_formatter(sink_formatter->clone());
      stderr_sink_->set_formatter(std::move(sink_formatter));
    }

   private:
    spdlog::sink_ptr stdout_sink_, stderr_sink_;
    unsigned int stderr_level_;
  };

 public:
  /// @brief Get the sink instance of spdlog
  spdlog::sink_ptr SpdSink() const { return sink_; }

  /// @brief Initializing the sink
  /// @param config Output config
  /// @return int   Return 0 means success and non-zero means failure
  int Init(const StdoutSinkConfig& config) {
    sink_ = std::make_shared<Sink>(config);
    return 0;
  }

 private:
  spdlog::sink_ptr sink_;
};

using StdoutSinkPtr = RefPtr<StdoutSink>;

}  // namespace trpc
