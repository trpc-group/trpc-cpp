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

#include "trpc/common/config/ssl_conf.h"
#include "trpc/transport/common/ssl/ssl.h"

namespace trpc::ssl {

/// @brief Initializes SSL
bool Init();

/// @brief Destroys SSL
void Destroy();


#ifdef TRPC_BUILD_INCLUDE_SSL

/// @brief Init SSL options for client-side
bool InitClientSslOptions(const ClientSslConfig& ssl_config, ClientSslOptions* ssl_options);

/// @brief Init SSL options for server-side
bool InitServerSslOptions(const ServerSslConfig& ssl_config, ServerSslOptions* ssl_options);

/// @brief create SSL for client-side
SslPtr CreateClientSsl(const SslContextPtr& ssl_ctx, const ClientSslOptions& ssl_options, int fd);

/// @brief Creates SSL for server-side
SslPtr CreateServerSsl(const SslContextPtr& ssl_ctx, const ServerSslOptions& ssl_options, int fd);

#endif

}  // namespace trpc::ssl
