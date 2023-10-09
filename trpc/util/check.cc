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

#include "trpc/util/check.h"

#include <string>
#include <vector>

namespace trpc::detail::log {

std::string StrError(int err) {
  char buf[100];
  char* ret = buf;
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE
  int rc = strerror_r(err, buf, sizeof(buf));
  if ((rc < 0) || (buf[0] == '\000')) {
    snprintf(buf, sizeof(buf), "Error number %d", err);
  }
#else
  errno = 0;
  ret = strerror_r(err, buf, sizeof(buf));
  if (errno != 0) {
    snprintf(buf, sizeof(buf), "Error number %d", err);
    ret = buf;
  }
#endif
  return ret;
}

}  // namespace trpc::detail::log
