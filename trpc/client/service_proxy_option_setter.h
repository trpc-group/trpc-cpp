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

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "trpc/client/service_proxy_option.h"
#include "trpc/common/config/mysql_client_conf.h"
#include "trpc/common/config/mysql_connect_pool_conf.h"
#include "trpc/common/config/redis_client_conf.h"
#include "trpc/common/config/ssl_conf.h"

namespace trpc {
namespace detail {

// Compare the input value with the default value to obtain the value actively configured by the user.
template <class T>
std::optional<T> GetValidInput(const T& input, const T& default_value) {
  if (default_value == input) {
    return std::nullopt;
  }
  return std::optional<T>(input);
}

// Assign the value set by the user to the output.
template <class T>
void SetOutputByValidInput(const std::optional<T>& input, T& output) {
  if (input.has_value()) {
    output = input.value();
  }
}

// ProxyCallback does not support setting from configuration file, so we assign directly here.
void SetOutputByValidInput(const ProxyCallback& input, ProxyCallback& output);

// If the std::vector<std::string> field isn't empty, it means that the user has set it and it needs to be assigned.
void SetOutputByValidInput(const std::vector<std::string>& input, std::vector<std::string>& output);

// If the std::any field isn't empty, it means that the user has set it and it needs to be assigned.
void SetOutputByValidInput(const std::any& input, std::any& output);

// If the field of RedisClientConf isn't the default value, it means that the user has set it and it needs to be
// assigned.
void SetOutputByValidInput(const RedisClientConf& input, RedisClientConf& output);

// If the field of MysqlClientConf isn't the default value, it means that the user has set it and it needs to be
// assigned.
void SetOutputByValidInput(const MysqlClientConf& input, MysqlClientConf& output);

// If the field of ClientSslConfig isn't the default value, it means that the user has set it and it needs to be
// assigned.
void SetOutputByValidInput(const ClientSslConfig& input, ClientSslConfig& output);

// Set the default value of ServiceProxyOption.
void SetDefaultOption(const std::shared_ptr<ServiceProxyOption>& option);

// Set the sepcified ServiceOption.
void SetSpecifiedOption(const ServiceProxyOption* option_ptr, const std::shared_ptr<ServiceProxyOption>& option);

}  // namespace detail
}  // namespace trpc
