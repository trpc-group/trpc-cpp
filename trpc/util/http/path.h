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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. seastar
// Copyright (C) 2015 Cloudius Systems
// seastar is licensed under the Apache 2.0 License.
//
//

#pragma once

#include <string>
#include <utility>

namespace trpc::http {
// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/include/seastar/http/json_path.hh.

/// @brief  The path of matching rule to match request URI's path.
class Path {
 public:
  explicit Path(std::string path) : path_(std::move(path)) {}
  Path(Path&&) = default;

  /// @brief Sets the parameter and returns the reference to itself.
  Path& Remainder(const std::string& param) {
    this->param_ = param;
    return *this;
  }

  /// @brief Gets the path.
  const std::string& GetPath() const { return path_; }

  /// @brief Sets the path.
  void SetPath(std::string path) { path_ = std::move(path); }

  /// @brief Gets the parameter.
  const std::string& GetParam() const { return param_; }

  /// @brief Gets the parameter.
  void SetParam(std::string param) { param_ = std::move(param); }

 private:
  std::string path_;
  std::string param_;
};
// End of source codes that are from seastar.

}  // namespace trpc::http
