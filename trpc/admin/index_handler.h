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

#pragma once

#include <map>
#include <string>

#include "trpc/admin/admin_handler.h"
#include "trpc/admin/web_css_jquery.h"

namespace trpc::admin {

/// @brief Handles the request for getting index or home page, then replies the content of service list.
class IndexHandler : public AdminHandlerBase {
 public:
  IndexHandler() { description_ = "[GET /] get admin service list"; }

  ~IndexHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  void PrintAhrefBody(std::string* html, const std::map<std::string, CmdInfo>& m_tabs);
};

}  // namespace trpc::admin
