//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include <jwt-cpp/jwt.h>

namespace trpc {

/// @brief Implementation of Json Web Token.
class Jwt {
 public:
  static bool isValid(const std::string& token, std::map<std::string, std::string>& auth_cfg);
};

}  // namespace trpc
