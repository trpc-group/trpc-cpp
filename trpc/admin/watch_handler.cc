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

#include "trpc/admin/watch_handler.h"

namespace trpc::admin {

void WatchHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                 rapidjson::Document::AllocatorType& alloc) {
  // FIXME: Implements it.
  result.AddMember("errorcode", "0", alloc);
  result.AddMember("message", "watching unsupported", alloc);
}

}  // namespace trpc::admin
