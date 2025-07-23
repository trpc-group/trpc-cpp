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

#include <stdarg.h>
#include <string>

#include "trpc/client/redis/request.h"

namespace trpc {

namespace redis {

/// @brief redis formatter when need format
/// @private For internal use purpose only.
class Formatter {
 public:
  /// @private For internal use purpose only.
  Formatter() = default;

  /// @private For internal use purpose only.
  ~Formatter() = default;

  /// @private For internal use purpose only.
  int FormatCommand(Request* req, const char* format, va_list ap);

 private:
  int TFormatCommand(Request* req, const char* format, ...);
  int VPrintf(std::string& current, const char* format, va_list ap);
  int TVPrintf(std::string& current, const char* format, ...);
};

}  // namespace redis

}  // namespace trpc
