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

#include "trpc/admin/commands_handler.h"

#include <utility>

namespace trpc::admin {

void CommandsHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                    rapidjson::Document::AllocatorType& alloc) {
  commands_function_(std::move(req), result, alloc);
}

}  // namespace trpc::admin
