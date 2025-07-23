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

#include "trpc/client/redis/redis_service_proxy.h"

#include <utility>

#include "trpc/codec/client_codec_factory.h"
#include "trpc/util/log/logging.h"
#include "trpc/transport/client/common/redis_client_io_handler.h"

namespace trpc {

namespace redis {

Status RedisServiceProxy::Command(const ClientContextPtr& context, Reply* rsp, const std::string& cmd) {
  Request req;

  req.do_RESP_ = false;
  req.params_.push_back(cmd);

  return UnaryInvoke<Request, Reply>(context, std::move(req), rsp);
}

Status RedisServiceProxy::Command(const ClientContextPtr& context, Reply* rsp, std::string&& cmd) {
  Request req;

  req.do_RESP_ = false;
  req.params_.emplace_back(std::move(cmd));

  return UnaryInvoke<Request, Reply>(context, std::move(req), rsp);
}

Status RedisServiceProxy::Command(const ClientContextPtr& context, Reply* rsp, const char* format, ...) {
  va_list ap;
  va_start(ap, format);

  Request req;
  if (formatter_->FormatCommand(&req, format, ap)) {
    va_end(ap);
    return Status(-1, "format command failed");
  }
  va_end(ap);

  return UnaryInvoke<Request, Reply>(context, std::move(req), rsp);
}

Future<Reply> RedisServiceProxy::AsyncCommand(const ClientContextPtr& context, std::string&& cmd) {
  Request req;
  req.do_RESP_ = false;
  req.params_.emplace_back(std::move(cmd));
  return AsyncUnaryInvoke<Request, Reply>(context, std::move(req));
}

Future<Reply> RedisServiceProxy::AsyncCommand(const ClientContextPtr& context, const std::string& cmd) {
  Request req;

  req.do_RESP_ = false;
  req.params_.push_back(cmd);

  return AsyncUnaryInvoke<Request, Reply>(context, std::move(req));
}

Future<Reply> RedisServiceProxy::AsyncCommand(const ClientContextPtr& context, const char* format, ...) {
  va_list ap;
  va_start(ap, format);

  Request req;
  if (formatter_->FormatCommand(&req, format, ap)) {
    va_end(ap);
    return MakeExceptionFuture<Reply>(CommonException("Format command faild"));
  }
  va_end(ap);

  return AsyncUnaryInvoke<Request, Reply>(context, std::move(req));
}

Status RedisServiceProxy::CommandArgv(const ClientContextPtr& context, const Request& req, Reply* rsp) {
  Request copy_req;
  copy_req.do_RESP_ = req.do_RESP_;
  copy_req.params_.reserve(req.params_.size());
  copy_req.params_.insert(copy_req.params_.begin(), req.params_.begin(), req.params_.end());
  return UnaryInvoke<Request, Reply>(context, std::move(copy_req), rsp);
}

Future<Reply> RedisServiceProxy::AsyncCommandArgv(const ClientContextPtr& context, const Request& req) {
  Request copy_req;
  copy_req.do_RESP_ = req.do_RESP_;
  copy_req.params_.reserve(req.params_.size());
  copy_req.params_.insert(copy_req.params_.begin(), req.params_.begin(), req.params_.end());
  return AsyncUnaryInvoke<Request, Reply>(context, std::move(copy_req));
}

Status RedisServiceProxy::CommandArgv(const ClientContextPtr& context, Request&& req, Reply* rsp) {
  return UnaryInvoke<Request, Reply>(context, std::move(req), rsp);
}

Future<Reply> RedisServiceProxy::AsyncCommandArgv(const ClientContextPtr& context, Request&& req) {
  return AsyncUnaryInvoke<Request, Reply>(context, std::move(req));
}

TransInfo RedisServiceProxy::ProxyOptionToTransInfo() {
  // codec MUST be in[redis,istore]
  TRPC_ASSERT((codec_->Name() == "redis" || codec_->Name() == "istore") && "protocol name must be redis or istore");
  TransInfo trans_info = ServiceProxy::ProxyOptionToTransInfo();
  // set option_->redis_conf so we can use it
  trans_info.user_data = GetServiceProxyOption()->redis_conf;
  return trans_info;
}

Status RedisServiceProxy::Command(const ClientContextPtr& context, std::string&& cmd) {
  Request req;

  req.do_RESP_ = false;
  req.params_.emplace_back(std::move(cmd));

  return OnewayInvoke<Request>(context, std::move(req));
}

Status RedisServiceProxy::Command(const ClientContextPtr& context, const std::string& cmd) {
  Request req;

  req.do_RESP_ = false;
  req.params_.push_back(cmd);

  return OnewayInvoke<Request>(context, std::move(req));
}

}  // namespace redis

}  // namespace trpc
