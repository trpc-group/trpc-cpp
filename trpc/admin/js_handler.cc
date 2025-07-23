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

#include "trpc/admin/js_handler.h"

#include "trpc/admin/web_css_jquery.h"

namespace trpc::admin {

JqueryJsHandler::JqueryJsHandler() {
  description_ = "[GET /cmdsweb/js/jquery_min] get jquery_min.js";
  jquery_min_ = JqueryMinJs();
}

void JqueryJsHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                    rapidjson::Document::AllocatorType& alloc) {
  result.AddMember("trpc-html", rapidjson::StringRef(jquery_min_), alloc);
}

VizJsHandler::VizJsHandler() {
  description_ = "[GET /cmdsweb/js/viz_min] get viz_min.js";
  viz_min_ = VizMinJs();
}

void VizJsHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                 rapidjson::Document::AllocatorType& alloc) {
  result.AddMember("trpc-html", rapidjson::StringRef(viz_min_), alloc);
}

FlotJsHandler::FlotJsHandler() {
  description_ = "[GET /cmdsweb/js/flot_min] get flot_min.js";
  flot_min_ = FlotMinJs();
}

void FlotJsHandler::CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                  rapidjson::Document::AllocatorType& alloc) {
  result.AddMember("trpc-html", rapidjson::StringRef(flot_min_), alloc);
}

}  // namespace trpc::admin
