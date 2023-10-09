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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. seastar
// Copyright (C) 2015 Cloudius Systems
// seastar is licensed under the Apache 2.0 License.
//
//

#include "trpc/util/http/routes.h"

#include "trpc/server/server_context.h"
#include "trpc/util/http/exception.h"

namespace trpc::http {
// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/src/http/routes.cc.

Routes& Routes::Add(std::shared_ptr<MatchRule> rule, MethodType type) {
  rules_[type].push_back(std::move(rule));
  return *this;
}

Routes& Routes::Add(MethodType type, const http::Path& path, std::shared_ptr<HandlerBase> handler) {
  auto rule = std::make_shared<MatchRule>(std::move(handler));
  rule->AddString(path.GetPath());
  if (!path.GetParam().empty()) {
    rule->AddParam(path.GetParam(), true);
  }
  return Add(std::move(rule), type);
}

HandlerBase* Routes::GetHandler(MethodType type, const std::string& path, Parameters& params) {
  HandlerBase* handler = GetExactMatch(type, path);
  if (handler != nullptr) {
    return handler;
  }
  for (const auto& rule : rules_[type]) {
    handler = rule->Get(path, params);
    if (handler != nullptr) {
      return handler;
    }
    params.Clear();
  }
  return nullptr;
}

HandlerBase* Routes::GetHandler(const std::string& path, RequestPtr& req) {
  return GetHandler(req->GetMethodType(), path, *req->GetMutableParameters());
}

trpc::Status Routes::Handle(const std::string& path, HandlerBase* handler, trpc::ServerContextPtr context,
                            RequestPtr& req, Response& rsp) {
  rsp.GenerateCommonReply(req.get());
  if (handler != nullptr) {
    try {
      // Parses query param from url and fill the query parameters.
      req->InitQueryParameters();
      return handler->Handle(path, std::move(context), req, &rsp);
    } catch (...) {
      ExceptionReply(std::current_exception(), &rsp);
    }
  } else {
    static const std::string not_found_ex = JsonException(NotFoundException("Not found")).ToJson();
    rsp.SetStatus(Response::StatusCode::kNotFound, not_found_ex);
    rsp.Done("json");
  }
  return kDefaultStatus;
}

void Routes::ExceptionReply(std::exception_ptr eptr, Response* rsp) {
  try {
    std::rethrow_exception(std::move(eptr));
  } catch (const BaseException& e) {
    rsp->SetStatus(e.Status(), JsonException(e).ToJson());
  } catch (...) {
    rsp->SetStatus(ResponseStatus::kInternalServerError,
                   JsonException(std::runtime_error("Unknown unhandled exception")).ToJson());
  }
  rsp->Done("json");
}
// End of source codes that are from seastar.

}  // namespace trpc::http
