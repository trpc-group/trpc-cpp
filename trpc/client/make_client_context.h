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

#include <functional>

#include "trpc/client/client_context.h"
#include "trpc/client/service_proxy.h"
#include "trpc/server/server_context.h"

namespace trpc {

/// @brief Create client context based on service proxy(for rpc).
/// @param proxy service proxy
/// @return client context
ClientContextPtr MakeClientContext(const ServiceProxyPtr& proxy);

/// @brief Create client context based on service proxy(for non-rpc).
/// @param proxy service proxy
/// @param req/rsp which is created by `std::make_shared<XxxProtocol>()`
/// @return client context
ClientContextPtr MakeClientContext(const ServiceProxyPtr& proxy, const ProtocolPtr& req, const ProtocolPtr& rsp);

/// @brief Create client context based on server context and service proxy(for rpc).
/// @param ctx server context
/// @param proxy service proxy
/// @param with_trans_info whether to copy the trans_info in the server context.
/// @return client context
ClientContextPtr MakeClientContext(const ServerContextPtr& ctx, const ServiceProxyPtr& proxy,
                                   bool with_trans_info = true);

/// @brief Create client context based on server context and service proxy(for non-rpc).
/// @param ctx server context
/// @param proxy service proxy
/// @param req/rsp which is created by `std::make_shared<XxxProtocol>()`
/// @param with_trans_info whether to copy the trans_info in the server context.
/// @return client context
ClientContextPtr MakeClientContext(const ServerContextPtr& ctx, const ServiceProxyPtr& proxy,
                                   const ProtocolPtr& req, const ProtocolPtr& rsp,
                                   bool with_trans_info = true);

/// @brief Create client context for transparent transmission.
/// @param ctx server context
/// @param proxy service proxy
/// @return client context
ClientContextPtr MakeTransparentClientContext(const ServerContextPtr& ctx, const ServiceProxyPtr& proxy);

/// @brief In the scenario of a route service, obtain the transparent transmission information replied by the backend
/// node, fill it back to the relay service, and then return it to the frontend node by the relay service.
/// @param[in] client_context client context
/// @param[in, out] server_context server context
void BackFillServerTransInfo(const ClientContextPtr& client_context, const ServerContextPtr& server_context);

/// @brief This is used by plugin developers to register how to set the contents of the server context into the client
///        context when create clientcontext using MakeClientContext (no thread-safe).
///        For example, a tracing plugin may use this to register how to set the span information in the client context.
using MakeClientContextCallback = std::function<void(const ServerContextPtr&, ClientContextPtr&)>;
void RegisterMakeClientContextCallback(MakeClientContextCallback&& callback);

}  // namespace trpc
