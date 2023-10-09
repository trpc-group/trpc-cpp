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

#include "trpc/common/config/ssl_conf.h"

#include <sstream>

#include "trpc/util/log/logging.h"

namespace trpc {

std::string SslConfig::ToString() const {
  std::ostringstream oss;
  oss << "ssl_enable:" << enable << std::endl;
  oss << "ssl_cert_path:" << cert_path << std::endl;
  oss << "ssl_ca_cert_path:" << ca_cert_path<< std::endl;
  oss << "ssl_private_key_path:" << private_key_path<< std::endl;
  oss << "ssl_cipher:" << ciphers << std::endl;
  oss << "ssl_dh_param_path:" << dh_param_path << std::endl;
  oss << "ssl_protocols:" << std::endl;
  for (auto iter = protocols.begin(); iter != protocols.end(); iter++) {
    oss << *iter;
    if (iter != protocols.end() -1) oss << ", ";
  }
  return oss.str();
}

void ClientSslConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");
  TRPC_LOG_DEBUG(SslConfig::ToString());
  TRPC_LOG_DEBUG("ssl_sni_name:" << sni_name);
  TRPC_LOG_DEBUG("ssl_insecure:" << insecure);
  TRPC_LOG_DEBUG("--------------------------------");
}

void ServerSslConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");
  TRPC_LOG_DEBUG(SslConfig::ToString());
  TRPC_LOG_DEBUG("mutual_auth" << mutual_auth);
  TRPC_LOG_DEBUG("--------------------------------");
}

}  // namespace trpc
