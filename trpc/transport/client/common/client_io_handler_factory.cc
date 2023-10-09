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

#include "trpc/transport/client/common/client_io_handler_factory.h"

#include "trpc/runtime/iomodel/reactor/common/default_io_handler.h"
#include "trpc/transport/client/common/client_io_handler.h"
#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/ssl.h"
#include "trpc/transport/common/ssl_helper.h"
#include "trpc/transport/common/ssl_io_handler.h"
#endif
#include "trpc/util/log/logging.h"

namespace trpc {

#ifdef TRPC_BUILD_INCLUDE_SSL
namespace ssl {
IoHandler* CreateIoHandler(const SslContextPtr& ssl_ctx, const ClientSslOptions& ssl_options, Connection* conn) {
  if (!ssl_ctx) return nullptr;
  // Creates SSL as client
  ssl::SslPtr ssl = CreateClientSsl(ssl_ctx, ssl_options, conn->GetFd());
  if (ssl != nullptr) {
    return new SslIoHandler(conn, std::move(ssl));
  }
  return nullptr;
}
}  // namespace ssl
#endif

namespace {

std::unique_ptr<IoHandler> CreateDefaultClientIoHandler(Connection* conn, TransInfo* trans_info) {
  std::unique_ptr<IoHandler> io_handler_ptr = nullptr;
  if (trans_info->custom_io_handler_function != nullptr) {
    io_handler_ptr.reset(new ClientIoHandler(conn, trans_info));
#ifdef TRPC_BUILD_INCLUDE_SSL
  } else if (trans_info->ssl_ctx && trans_info->ssl_options) {
    // !!! Note: nullptr would be returned here when error has occurred.
    io_handler_ptr.reset(ssl::CreateIoHandler(trans_info->ssl_ctx, *(trans_info->ssl_options), conn));
#endif
  } else {
    io_handler_ptr.reset(new DefaultIoHandler(conn));
  }

  return io_handler_ptr;
}

}  // namespace

bool ClientIoHandlerFactory::Register(const std::string& protocol, ClientIoHandlerCreator&& creator) {
  auto it = io_handlers_.find(protocol);
  if (it != io_handlers_.end()) {
    return false;
  }

  io_handlers_.emplace(protocol, std::move(creator));
  return true;
}

void ClientIoHandlerFactory::Clear() { io_handlers_.clear(); }

std::unique_ptr<IoHandler> ClientIoHandlerFactory::Create(const std::string& protocol, Connection* conn,
                                                          TransInfo* trans_info) {
  std::unique_ptr<IoHandler> io_handler;

  auto it = io_handlers_.find(protocol);
  if (it != io_handlers_.end()) {
    io_handler = (it->second)(conn, trans_info);
  } else {
    io_handler = CreateDefaultClientIoHandler(conn, trans_info);
  }

  return io_handler;
}

}  // namespace trpc
