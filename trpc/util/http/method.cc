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

#include "trpc/util/http/method.h"

#include <array>

namespace trpc::http {

constexpr char kMethodGet[] = "GET";
constexpr char kMethodHead[] = "HEAD";
constexpr char kMethodPost[] = "POST";
constexpr char kMethodPut[] = "PUT";
constexpr char kMethodDelete[] = "DELETE";
constexpr char kMethodOptions[] = "OPTIONS";  // RFC 5789
constexpr char kMethodPatch[] = "PATCH";
constexpr char kMethodUnknown[] = "";

const std::string& MethodName(MethodType type) {
  const static std::array<std::string, MethodType::UNKNOWN + 1> method_names = {
      kMethodGet, kMethodHead, kMethodPost, kMethodPut, kMethodDelete, kMethodOptions, kMethodPatch,
  };
  return method_names[type];
}

MethodType MethodNameToMethodType(std::string_view name) {
  // POST and GET are most commonly used, so for performance, they are the top.
  if (name == kMethodPost) {
    return MethodType::POST;
  }

  if (name == kMethodGet) {
    return MethodType::GET;
  }

  if (name == kMethodHead) {
    return MethodType::HEAD;
  }

  if (name == kMethodPut) {
    return MethodType::PUT;
  }

  if (name == kMethodDelete) {
    return MethodType::DELETE;
  }

  if (name == kMethodOptions) {
    return MethodType::OPTIONS;
  }

  if (name == kMethodPatch) {
    return MethodType::PATCH;
  }

  return MethodType::UNKNOWN;
}

const std::string& TypeName(OperationType type) { return MethodName(type); }

OperationType StringToType(std::string_view name) { return MethodNameToMethodType(name); }

}  // namespace trpc::http
