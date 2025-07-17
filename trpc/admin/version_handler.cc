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

#include "trpc/admin/version_handler.h"

#include "trpc/common/trpc_version.h"

namespace trpc::admin {

void VersionHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                   rapidjson::Document::AllocatorType& alloc) {
  const char* trpc_version = TRPC_Cpp_Version();
  result.AddMember(rapidjson::StringRef("version"), rapidjson::StringRef(trpc_version), alloc);
}

}  // namespace trpc::admin
