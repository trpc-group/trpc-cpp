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

#include <string>

#include "trpc/admin/admin_handler.h"

namespace trpc::admin {

/// @brief Handles the request for getting jquery-min.js, then replies the content of jquery-min.js.
class JqueryJsHandler : public AdminHandlerBase {
 public:
  JqueryJsHandler();

  ~JqueryJsHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  std::string jquery_min_;
};

/// @brief Handles the request for getting viz-min.js, then replies the content of viz-min.js.
class VizJsHandler : public AdminHandlerBase {
 public:
  VizJsHandler();

  ~VizJsHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  std::string viz_min_;
};

/// @brief Handles the request for getting flot-min.js, then replies the content of flot-min.js.
class FlotJsHandler : public AdminHandlerBase {
 public:
  FlotJsHandler();

  ~FlotJsHandler() override = default;

  void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                     rapidjson::Document::AllocatorType& alloc) override;

 private:
  std::string flot_min_;
};

}  // namespace trpc::admin
