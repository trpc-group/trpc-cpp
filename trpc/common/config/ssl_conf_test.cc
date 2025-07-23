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


#include "trpc/common/config/ssl_conf.h"

#include "gtest/gtest.h"

#include "trpc/common/config/ssl_conf_parser.h"

namespace trpc::testing {

class ServerSslConfigTest : public ::testing::Test {
 public:
  void SetUp() override {
    // nit ssl config
    ssl_config_.enable = true;
    ssl_config_.mutual_auth = true;
    ssl_config_.ca_cert_path = "/path/to/xx_ca_cert.pem";
    ssl_config_.cert_path = "/path/to/xx_cert.pem";
    ssl_config_.private_key_path = "/path/to/xx_key.pem";
    ssl_config_.ciphers = R"(DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT)";
    ssl_config_.dh_param_path = "/path/to/xx_dh_param.dhparam";
    ssl_config_.protocols.emplace_back("SSLv3");
    ssl_config_.protocols.emplace_back("TLSv1");
    ssl_config_.protocols.emplace_back("TLSv1.1");
    ssl_config_.protocols.emplace_back("TLSv1.2");
  }

  void TearDown() override {}

 protected:
  ServerSslConfig ssl_config_;
};

TEST_F(ServerSslConfigTest, EncodeAndDecode) {
  ServerSslConfig decoded_ssl_config;
  YAML::Node yaml_node = YAML::convert<ServerSslConfig>::encode(ssl_config_);

  ASSERT_TRUE(YAML::convert<ServerSslConfig>::decode(yaml_node, decoded_ssl_config));

  ASSERT_EQ(ssl_config_.enable, decoded_ssl_config.enable);
  ASSERT_EQ(ssl_config_.mutual_auth, decoded_ssl_config.mutual_auth);
  ASSERT_EQ(ssl_config_.ca_cert_path, decoded_ssl_config.ca_cert_path);
  ASSERT_EQ(ssl_config_.cert_path, decoded_ssl_config.cert_path);
  ASSERT_EQ(ssl_config_.private_key_path, decoded_ssl_config.private_key_path);
  ASSERT_EQ(ssl_config_.ciphers, decoded_ssl_config.ciphers);
  ASSERT_EQ(ssl_config_.dh_param_path, decoded_ssl_config.dh_param_path);
  ASSERT_EQ(ssl_config_.protocols.size(), decoded_ssl_config.protocols.size());

  decoded_ssl_config.Display();
}

class ClientSslConfigTest : public ::testing::Test {
 public:
  void SetUp() override {
    // nit ssl config
    ssl_config_.enable = true;
    ssl_config_.sni_name = "www.xxops.com";
    ssl_config_.ca_cert_path = "/path/to/xx_ac_cert.pem";
    ssl_config_.cert_path = "/path/to/xx_cert.pem";
    ssl_config_.private_key_path = "/path/to/xx_key.pem";
    ssl_config_.ciphers = R"(DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT)";
    ssl_config_.dh_param_path = "/path/to/xx_dh_param.dhparam";
    ssl_config_.protocols.emplace_back("SSLv3");
    ssl_config_.protocols.emplace_back("TLSv1");
    ssl_config_.protocols.emplace_back("TLSv1.1");
    ssl_config_.protocols.emplace_back("TLSv1.2");
    ssl_config_.insecure = false;
  }

  void TearDown() override {}

 protected:
  ClientSslConfig ssl_config_;
};

TEST_F(ClientSslConfigTest, EncodeAndDecode) {
  ClientSslConfig decoded_ssl_config;
  YAML::Node yaml_node = YAML::convert<ClientSslConfig>::encode(ssl_config_);

  ASSERT_TRUE(YAML::convert<ClientSslConfig>::decode(yaml_node, decoded_ssl_config));

  ASSERT_EQ(ssl_config_.enable, decoded_ssl_config.enable);
  ASSERT_EQ(ssl_config_.sni_name, decoded_ssl_config.sni_name);
  ASSERT_EQ(ssl_config_.ca_cert_path, decoded_ssl_config.ca_cert_path);
  ASSERT_EQ(ssl_config_.cert_path, decoded_ssl_config.cert_path);
  ASSERT_EQ(ssl_config_.private_key_path, decoded_ssl_config.private_key_path);
  ASSERT_EQ(ssl_config_.ciphers, decoded_ssl_config.ciphers);
  ASSERT_EQ(ssl_config_.dh_param_path, decoded_ssl_config.dh_param_path);
  ASSERT_EQ(ssl_config_.protocols.size(), decoded_ssl_config.protocols.size());

  decoded_ssl_config.Display();
}

}  // namespace trpc::testing
