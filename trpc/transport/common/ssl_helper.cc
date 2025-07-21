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

#include "trpc/transport/common/ssl_helper.h"

#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl/core.h"
#endif
#include "trpc/util/log/logging.h"

namespace trpc::ssl {

bool Init() {
#ifdef TRPC_BUILD_INCLUDE_SSL
  if (InitOpenSsl()) {
    TRPC_LOG_DEBUG("init openssl succeed.");
    return true;
  } else {
    TRPC_LOG_ERROR("init openssl failed.");
    return false;
  }
#endif

  return true;
}

void Destroy() {
#ifdef TRPC_BUILD_INCLUDE_SSL
  DestroyOpenSsl();

  TRPC_LOG_DEBUG("destroy openssl.");
#endif
}

#ifdef TRPC_BUILD_INCLUDE_SSL

bool InitClientSslOptions(const ClientSslConfig& ssl_config, ClientSslOptions* ssl_options) {
  if (ssl_config.enable) {
    ssl_options->client_cert.cert_path = ssl_config.cert_path;
    ssl_options->client_cert.private_key_path = ssl_config.private_key_path;
    ssl_options->sni_name = ssl_config.sni_name;
    ssl_options->ciphers = ssl_config.ciphers;
    ssl_options->dh_param_path = ssl_config.dh_param_path;
    ssl_options->insecure = ssl_config.insecure;
    ssl_options->verify_peer_options.ca_cert_path = ssl_config.ca_cert_path;
    // Convert protocols string to protocols value
    ssl_options->protocols = ParseProtocols(ssl_config.protocols);

    // Sets default CA cert path.
    if (ssl_options->verify_peer_options.ca_cert_path.empty()) {
      ssl_options->verify_peer_options.ca_cert_path = "/etc/pki/tls/certs/ca-bundle.crt";
    }
  }
  return true;
}

bool InitServerSslOptions(const ServerSslConfig& ssl_config, ServerSslOptions* ssl_options) {
  if (ssl_config.enable) {
    ssl_options->default_cert.cert_path = ssl_config.cert_path;
    ssl_options->default_cert.private_key_path = ssl_config.private_key_path;
    ssl_options->ciphers = ssl_config.ciphers;
    ssl_options->dh_param_path = ssl_config.dh_param_path;
    ssl_options->enable_verify_peer = ssl_config.mutual_auth;
    ssl_options->verify_peer_options.ca_cert_path = ssl_config.ca_cert_path;
    // Convert protocols string to protocols value
    ssl_options->protocols = ParseProtocols(ssl_config.protocols);
  }
  return true;
}

SslPtr CreateClientSsl(const SslContextPtr& ssl_ctx, const ClientSslOptions& ssl_options, int fd) {
  // Create SSL
  SslPtr ssl = ssl_ctx->NewSsl();
  if (ssl == nullptr) return nullptr;

  // Mount fd of connection to ssl
  if (fd >= 0) {
    if (!ssl->SetFd(fd)) return nullptr;
  }

  // Set SNI server_name
  if (!ssl_options.sni_name.empty()) {
    if (!ssl->SetTlsExtensionServerName(ssl_options.sni_name)) return nullptr;
  }
  // Set ssl to work in client mode
  ssl->SetConnectState();
  return ssl;
}

SslPtr CreateServerSsl(const SslContextPtr& ssl_ctx, const ServerSslOptions& ssl_options, int fd) {
  // Create SSL
  SslPtr ssl = ssl_ctx->NewSsl();
  if (ssl == nullptr) return nullptr;

  // Mount fd of connection to ssl
  if (fd >= 0) {
    if (!ssl->SetFd(fd)) return nullptr;
  }
  // Set ssl to work in server mode
  ssl->SetAcceptState();
  return ssl;
}

#endif

}  // namespace trpc::ssl
