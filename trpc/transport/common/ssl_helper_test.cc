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

#ifdef TRPC_BUILD_INCLUDE_SSL
#include "trpc/transport/common/ssl_helper.h"

#include "gtest/gtest.h"

#include "trpc/common/config/ssl_conf.h"
#include "trpc/transport/common/ssl/core.h"
#include "trpc/transport/common/ssl/ssl.h"

namespace trpc::testing {

class SslHelperTest : public ::testing::Test {
 protected:
  void SetUp() override {
    assert(ssl::InitOpenSsl());

    InitSslConfig();
  }

  void TearDown() override { ssl::DestroyOpenSsl(); }

  void InitSslConfig() {
    client_ssl_config_.enable = true;
    client_ssl_config_.sni_name = "www.xxops.com";
    client_ssl_config_.ca_cert_path = "./trpc/transport/common/ssl/cert/xxops-com-chain.pem";
    client_ssl_config_.ciphers = "HIGH:!aNULL:!kRSA:!SRP:!PSK:!CAMELLIA:!RC4:!MD5:!DSS";
    client_ssl_config_.dh_param_path = "./trpc/transport/common/ssl/cert/xxops-com.dhparam";
    client_ssl_config_.protocols.emplace_back("TLSv1.1");
    client_ssl_config_.protocols.emplace_back("TLSv1.2");
    client_ssl_config_.insecure = true;

    server_ssl_config_.enable = true;
    server_ssl_config_.cert_path = "./trpc/transport/common/ssl/cert/server_cert.pem";
    server_ssl_config_.private_key_path = "./trpc/transport/common/ssl/cert/server_key.pem";
    server_ssl_config_.ciphers = "HIGH:!aNULL:!kRSA:!SRP:!PSK:!CAMELLIA:!RC4:!MD5:!DSS";
    server_ssl_config_.dh_param_path = "./trpc/transport/common/ssl/cert/server_dhparam.pem";
    server_ssl_config_.protocols.emplace_back("TLSv1.1");
    server_ssl_config_.protocols.emplace_back("TLSv1.2");
  }

 protected:
  ClientSslConfig client_ssl_config_;
  ServerSslConfig server_ssl_config_;
};

TEST_F(SslHelperTest, InitClientSsl) {
  ssl::ClientSslOptions ssl_options;
  EXPECT_TRUE(ssl::InitClientSslOptions(client_ssl_config_, &ssl_options));

  ssl::SslContextPtr ssl_ctx = MakeRefCounted<ssl::SslContext>();
  EXPECT_TRUE(ssl_ctx->Init(ssl_options));

  ssl::SslPtr ssl = CreateClientSsl(ssl_ctx, ssl_options, -1);
  EXPECT_TRUE(ssl != nullptr);
}

TEST_F(SslHelperTest, InitServerSsl) {
  ssl::ServerSslOptions ssl_options;
  EXPECT_TRUE(ssl::InitServerSslOptions(server_ssl_config_, &ssl_options));

  ssl::SslContextPtr ssl_ctx = MakeRefCounted<ssl::SslContext>();
  EXPECT_TRUE(ssl_ctx->Init(ssl_options));

  ssl::SslPtr ssl = CreateServerSsl(ssl_ctx, ssl_options, -1);
  EXPECT_TRUE(ssl != nullptr);
}

}  // namespace trpc::testing
#endif
