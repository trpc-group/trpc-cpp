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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. seastar
// Copyright (C) 2015 Cloudius Systems
// seastar is licensed under the Apache 2.0 License.
//
//

#include "trpc/util/http/mime_types.h"

#include <unordered_map>

namespace trpc::http {
// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/src/http/mime_types.cc.

namespace {
const std::unordered_map<std::string, std::string> kMimeTypes = {
    {"json", "application/json"},
    {"gif", "image/gif"},
    {"htm", "text/html"},
    {"css", "text/css"},
    {"js", "text/javascript"},
    {"html", "text/html"},
    {"jpg", "image/jpeg"},
    {"png", "image/png"},
    {"txt", "text/plain"},
    {"ico", "image/x-icon"},
    {"bin", "application/octet-stream"},
    {"proto", "application/proto"},
};
}  // namespace

std::string ExtensionToType(const std::string& extension) {
  auto it = kMimeTypes.find(extension);
  if (it != kMimeTypes.end()) {
    return it->second;
  }
  return "text/plain";
}
// End of source codes that are from seastar.

}  // namespace trpc::http
