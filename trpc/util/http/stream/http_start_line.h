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

#include "trpc/util/http/common.h"

namespace trpc::stream {

/// @brief Request line of HTTP request.
/// @private For internal use purpose only.
struct HttpRequestLine {
  std::string method{"GET"};
  std::string request_uri{"/"};
  std::string version{trpc::http::kVersion11};
};

/// @brief Status line of HTTP response.
/// @private For internal use purpose only.
struct HttpStatusLine {
  int status_code{200};
  std::string status_text{"OK"};
  std::string version{trpc::http::kVersion11};
};

}  // namespace trpc::stream
