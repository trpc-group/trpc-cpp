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

#include <string>

namespace trpc::http {

/// @brief HTTP method type.
/// Unless otherwise noted, these are defined in RFC 7321 section 4.3.
/// RFC : https://www.rfc-editor.org/rfc/rfc7231#section-4.3
enum MethodType {
  GET = 0,
  HEAD,
  POST,
  PUT,
  DELETE,
  OPTIONS,
  PATCH,
  UNKNOWN,
};
/// @brief Deprecated: use MethodType instead.
using OperationType = MethodType;

/// @brief Returns literal value of HTTP method. Empty string returned if not not match.
const std::string& MethodName(MethodType type);
/// @brief Deprecated: use MethodName instead.
const std::string& TypeName(OperationType type);

/// @brief Converts case-sensitive string "str" to enum Method. MethodType::UNKNOWN returned if not found.
MethodType MethodNameToMethodType(std::string_view name);
/// @brief Deprecated: use MethodNameToMethodType instead.
OperationType StringToType(std::string_view name);

}  // namespace trpc::http
