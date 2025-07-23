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

#include "trpc/common/status.h"

#include <cstdio>  // snprintf

namespace trpc {

std::string Status::ToString() const {
  if (OK()) {
    return "OK";
  }
  char tmp[50];
  std::snprintf(tmp, sizeof(tmp), "ret: %d, func_ret: %d, err: ", ret_, func_ret_);

  return std::string(tmp) + error_message_;
}

}
