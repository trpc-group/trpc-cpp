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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "trpc/admin/admin_handler.h"

namespace trpc::admin {

/// @brief Handles the request for detaching connection that is connected to target IP:PORT.
class ClientDetachHandler : public AdminHandlerBase {
 public:
  ClientDetachHandler() { description_ = "[POST /client_detach] client detach from target"; }

  ~ClientDetachHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;
};

}  // namespace trpc::admin
