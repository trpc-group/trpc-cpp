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

#include <memory>
#include <string>

#include "trpc/admin/admin_handler.h"

namespace trpc::admin {

/// @brief Handles the request for reloading server configure dynamically, then replies the result reloading action.
class ReloadConfigHandler : public AdminHandlerBase {
 public:
  ReloadConfigHandler() { description_ = "[POST /reload-config] reload trpc server config. (content eg. {})"; }

  ~ReloadConfigHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

/// @brief Handles the request for displaying server configure dynamically, then replies the content of configure.
class WebReloadConfigHandler : public AdminHandlerBase {
 public:
  WebReloadConfigHandler() { description_ = "[POST /cmdsweb/config] show trpc server config"; }

  ~WebReloadConfigHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

}  // namespace trpc::admin
