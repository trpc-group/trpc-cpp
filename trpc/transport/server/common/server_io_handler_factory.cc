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

#include "trpc/transport/server/common/server_io_handler_factory.h"

#include "trpc/runtime/iomodel/reactor/common/default_io_handler.h"
#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/ssl.h"
#include "trpc/transport/common/ssl_helper.h"
#include "trpc/transport/common/ssl_io_handler.h"
#endif

namespace trpc {

#ifdef TRPC_BUILD_INCLUDE_SSL

namespace ssl {

IoHandler* CreateIoHandler(const SslContextPtr& ssl_ctx, const ServerSslOptions& ssl_options,
                           Connection* conn) {
  if (!ssl_ctx) return nullptr;
  // Creates SSL as server
  SslPtr ssl = CreateServerSsl(ssl_ctx, ssl_options, conn->GetFd());
  if (ssl != nullptr) {
    return new SslIoHandler(conn, std::move(ssl));
  }
  return nullptr;
}

}  // namespace ssl

#endif

namespace {

std::unique_ptr<IoHandler> CreateIoHandler(Connection* conn, const BindInfo& bind_info) {
  std::unique_ptr<IoHandler> io_handler_ptr = nullptr;

#ifdef TRPC_BUILD_INCLUDE_SSL
  if (bind_info.ssl_ctx && bind_info.ssl_options) {
    IoHandler* ptr = ssl::CreateIoHandler(bind_info.ssl_ctx, bind_info.ssl_options.value(), conn);
    TRPC_ASSERT(ptr != nullptr);
    io_handler_ptr.reset(ptr);
  } else {
#endif
    io_handler_ptr.reset(new DefaultIoHandler(conn));
#ifdef TRPC_BUILD_INCLUDE_SSL
  }
#endif

  return io_handler_ptr;
}

}  // namespace

bool ServerIoHandlerFactory::Register(const std::string& protocol, ServerIoHandlerCreator&& creator) {
  auto it = io_handlers_.find(protocol);
  if (it != io_handlers_.end()) {
    return false;
  }

  io_handlers_.emplace(protocol, std::move(creator));

  return true;
}

std::unique_ptr<IoHandler> ServerIoHandlerFactory::Create(const std::string& protocol, Connection* conn,
                                                    const BindInfo& bind_info) {
  std::unique_ptr<IoHandler> io_handler;

  auto it = io_handlers_.find(protocol);
  if (it != io_handlers_.end()) {
    io_handler = (it->second)(conn, bind_info);
  } else {
    io_handler = CreateIoHandler(conn, bind_info);
  }

  return io_handler;
}

void ServerIoHandlerFactory::Clear() { io_handlers_.clear(); }

}  // namespace trpc
