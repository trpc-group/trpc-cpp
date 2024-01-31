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

#include <cstdint>
#include <string>

#include "yaml-cpp/yaml.h"

namespace trpc::naming {

struct DefaultCircuitBreakerConfig {
  bool enable{false};                          // Whether to enable circuit breaker
  uint32_t stat_window_ms{60000};              // Statistical time window period (in milliseconds)
  uint32_t buckets_num{12};                    // Buckets number in statistical time window period
  uint32_t sleep_window_ms{30000};             // The time it takes to switch to the half-open state from open state
  uint32_t request_volume_threshold{10};       // The minimum number of requests required to trigger circuit breaker
  float error_rate_threshold{0.5};             // The percentage of failure rate at which circuit breaker is triggered
  uint32_t continuous_error_threshold{10};     // The number of consecutive failures before triggering circuit breaker
  uint32_t request_count_after_half_open{10};  // The number of allowed requests in the half-open state
  uint32_t success_count_after_half_open{8};   // The minimum number of successful attempts required to transition from
                                               // the half-open state to the closed state.
};

}  // namespace trpc::naming

// convert strings ending with 'ms' or 's' into milliseconds
static uint32_t ConvertToMilliSeconds(const std::string& time_str) {
  uint32_t time_ms = 0;
  auto str_size = time_str.size();
  // strings ending with 'ms' or 's'
  if (str_size >= 2) {
    if (!time_str.substr(str_size - 2).compare("ms")) {
      time_ms = atoi(time_str.substr(0, str_size - 2).c_str());
      return time_ms;
    } else if (!time_str.substr(str_size - 1).compare("s")) {
      time_ms = atoi(time_str.substr(0, str_size - 1).c_str()) * 1000;
      return time_ms;
    }
  }

  // strings without time unit
  time_ms = atoi(time_str.c_str());
  return time_ms;
}

namespace YAML {

template <>
struct convert<trpc::naming::DefaultCircuitBreakerConfig> {
  static YAML::Node encode(const trpc::naming::DefaultCircuitBreakerConfig& config) {
    YAML::Node node;
    node["enable"] = config.enable;
    node["statWindow"] = std::to_string(config.stat_window_ms) + "ms";
    node["bucketsNum"] = config.buckets_num;
    node["sleepWindow"] = std::to_string(config.sleep_window_ms) + "ms";
    node["requestVolumeThreshold"] = config.request_volume_threshold;
    node["errorRateThreshold"] = config.error_rate_threshold;
    node["continuousErrorThreshold"] = config.continuous_error_threshold;
    node["requestCountAfterHalfOpen"] = config.request_count_after_half_open;
    node["successCountAfterHalfOpen"] = config.success_count_after_half_open;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::DefaultCircuitBreakerConfig& config) {
    if (node["enable"]) {
      config.enable = node["enable"].as<bool>();
    }
    if (node["statWindow"]) {
      std::string stat_window = node["statWindow"].as<std::string>();
      config.stat_window_ms = ConvertToMilliSeconds(stat_window);
    }
    if (node["bucketsNum"]) {
      config.buckets_num = node["bucketsNum"].as<uint32_t>();
    }

    if (config.stat_window_ms % config.buckets_num != 0) {
      config.stat_window_ms = config.stat_window_ms / config.buckets_num * config.buckets_num;
    }

    if (node["sleepWindow"]) {
      std::string sleep_window = node["sleepWindow"].as<std::string>();
      config.sleep_window_ms = ConvertToMilliSeconds(sleep_window);
    }
    if (node["requestVolumeThreshold"]) {
      config.request_volume_threshold = node["requestVolumeThreshold"].as<uint32_t>();
    }
    if (node["errorRateThreshold"]) {
      config.error_rate_threshold = node["errorRateThreshold"].as<float>();
    }
    if (node["continuousErrorThreshold"]) {
      config.continuous_error_threshold = node["continuousErrorThreshold"].as<uint32_t>();
      if (config.continuous_error_threshold == 0) {
        config.continuous_error_threshold = 1;
      }
    }
    if (node["requestCountAfterHalfOpen"]) {
      config.request_count_after_half_open = node["requestCountAfterHalfOpen"].as<uint32_t>();
    }
    if (node["successCountAfterHalfOpen"]) {
      config.success_count_after_half_open = node["successCountAfterHalfOpen"].as<uint32_t>();
    }
    return true;
  }
};

}  // namespace YAML
