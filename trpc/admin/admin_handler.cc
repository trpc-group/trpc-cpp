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

#include "trpc/admin/admin_handler.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace trpc {

trpc::Status AdminHandlerBase::Handle(const std::string& path, trpc::ServerContextPtr context, http::HttpRequestPtr req,
                                      http::HttpResponse* rsp) {
  rapidjson::Document doc;
  rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
  rapidjson::Value result(rapidjson::kObjectType);

  // Executes detail command handler.
  CommandHandle(std::move(req), result, alloc);

  if (result.HasMember("trpc-html")) {
    rsp->SetContent(result["trpc-html"].GetString());
    rsp->Done("html");
  } else {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    result.Accept(writer);
    rsp->SetContent(buffer.GetString());
    rsp->Done("json");
  }
  return kDefaultStatus;
}

}  // namespace trpc
