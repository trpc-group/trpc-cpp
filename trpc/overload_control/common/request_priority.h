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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#pragma once

#include <functional>

#include "trpc/client/client_context.h"
#include "trpc/server/server_context.h"
#include "trpc/util/function.h"

namespace trpc::overload_control {

/// @brief Method for the server to retrieve the request priority.
using ServerGetPriorityFunc = Function<int(const ServerContextPtr& context)>;
/// @brief Method for the client to retrieve the request priority.
using ClientGetPriorityFunc = Function<int(const ClientContextPtr& context)>;

/// @brief Setting the global method for the server to retrieve the request priority.
/// @param func User-defined server priority settings.
void SetServerGetPriorityFunc(ServerGetPriorityFunc&& func);
/// @brief Setting the global method for the client to retrieve the request priority.
/// @param func User-defined client priority settings.
void SetClientGetPriorityFunc(ClientGetPriorityFunc&& func);

/// @brief Using a registered function to retrieve the priority from the server.
/// @param context Context of server
/// @return The value representing the server priority is of type int.
int GetServerPriority(const ServerContextPtr& context);
/// @brief Using a registered function to retrieve the priority from the client.
/// @param context Context of client
/// @return The value representing the client priority is of type int.
int GetClientPriority(const ClientContextPtr& context);

/// @brief The default method for retrieving the server request priority in the framework.
/// @param context Context of server
/// @return The value representing the server priority is of type int.
int DefaultGetServerPriority(const ServerContextPtr& context);
/// @brief The default method for retrieving the client request priority in the framework.
/// @param context Context of client
/// @return The value representing the client priority is of type int.
int DefaultGetClientPriority(const ClientContextPtr& context);

/// @brief The default method for setting the server request priority in the framework.
/// @param context Context of server
/// @param priority Default priority
void DefaultSetServerPriority(const ServerContextPtr& context, int priority);
/// @brief The default method for setting the client request priority in the framework.
/// @param context Context of client
/// @param priority Default priority
void DefaultSetClientPriority(const ClientContextPtr& context, int priority);

}  // namespace trpc::overload_control

#endif
