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

#include <string>

namespace trpc {

/// @brief Method type
enum class MethodType {
  UNARY = 0,
  CLIENT_STREAMING = 1,
  SERVER_STREAMING = 2,
  BIDI_STREAMING = 3,
};

/// @brief Base class of method。
class Method {
 public:
  Method(const std::string& name, MethodType type) : name_(name), method_type_(type) {}

  virtual ~Method() = default;

  /// @brief Get name of method
  /// @return std::string
  /// @note The return value string here is copy-on-write.
  std::string Name() const { return name_; }

  /// @brief Get type of method
  /// @return MethodType
  MethodType GetMethodType() const { return method_type_; }

  /// @brief Set type of method
  /// @param type MethodType
  void SetMethodType(MethodType type) { method_type_ = type; }

 private:
  std::string name_;

  MethodType method_type_;
};

}  // namespace trpc
