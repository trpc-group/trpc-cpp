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

#include <functional>
#include <memory>

#include "trpc/admin/admin_handler.h"

namespace trpc::admin {

using CommandsFunction =
    std::function<void(http::HttpRequestPtr, rapidjson::Value&, rapidjson::Document::AllocatorType&)>;

class CommandsHandler : public AdminHandlerBase {
 public:
  explicit CommandsHandler(CommandsFunction commands_function) : commands_function_(commands_function) {
    description_ = "[GET /cmds]      list supported cmds";
  }

  ~CommandsHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  CommandsFunction commands_function_;
};

}  // namespace trpc::admin
