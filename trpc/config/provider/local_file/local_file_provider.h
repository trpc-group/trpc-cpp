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

#include <sys/stat.h>
#include <sys/types.h>

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <thread>

#include "trpc/common/config/config_helper.h"
#include "trpc/common/config/local_file_provider_conf_parser.h"
#include "trpc/config/default/loader.h"
#include "trpc/config/provider.h"

namespace trpc::config {

class LocalFileProvider : public Provider {
 public:
  explicit LocalFileProvider(LocalFileProviderConfig config)
      : name_(config.name),
        filename_(ConfigHelper::ExpandEnv(config.filename)),
        last_modified_time_(LastWriteTime(ConfigHelper::ExpandEnv(config.filename))),
        config_(std::move(config)),
        polling_thread_([this] {
          std::unique_lock lock(stop_mutex_);
          while (!stop_cv_.wait_for(lock, std::chrono::seconds(1), [this] { return stop_flag_; })) {
            std::unique_lock callback_lock(callback_mutex_);
            if (callbacks_.empty()) continue;
            auto mod_time = LastWriteTime(config_.filename);
            if (mod_time != last_modified_time_) {
              std::string content = Read("");
              for (const auto& callback : callbacks_) {
                callback(config_.name, content);
              }
              last_modified_time_ = mod_time;
            }
          }
        }) {}

  ~LocalFileProvider() override {
    {
      std::unique_lock lock(stop_mutex_);
      stop_flag_ = true;
    }
    stop_cv_.notify_one();
    polling_thread_.join();
  }

  std::string Name() const override { return name_; }

  std::string Read(const std::string&) override {
    std::ifstream in{filename_};
    if (!in) {
      TRPC_LOG_ERROR("Unable to read local file config: " << config_.filename);
      return "";
    }
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
  }

  void Watch(trpc::config::ProviderCallback callback) override {
    std::unique_lock callback_lock(callback_mutex_);
    callbacks_.emplace_back(callback);
  }

 private:
  static std::int64_t LastWriteTime(const std::string& filename) {
    struct stat result {};
    if (stat(filename.c_str(), &result) == 0) {
      return result.st_mtim.tv_sec * 100000ll + result.st_mtim.tv_nsec;
    }
    return 0;
  }

  std::string name_;
  std::string filename_;
  std::int64_t last_modified_time_;
  LocalFileProviderConfig config_;

  std::condition_variable stop_cv_;
  std::mutex stop_mutex_;
  bool stop_flag_{false};
  std::thread polling_thread_;

  std::mutex callback_mutex_;
  std::vector<trpc::config::ProviderCallback> callbacks_;
};

};  // namespace trpc::config
