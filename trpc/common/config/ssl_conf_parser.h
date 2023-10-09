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

#include <string>

#include "yaml-cpp/yaml.h"

#include "trpc/common/config/ssl_conf.h"

namespace YAML {

/// A helper to parse configure content of SSL.
template <>
struct convert<trpc::SslConfig> {
  static void encode(const trpc::SslConfig& ssl_config, YAML::Node& node) {
    node["enable"] = ssl_config.enable;
    node["cert_path"] = ssl_config.cert_path;
    node["ca_cert_path"] = ssl_config.ca_cert_path;
    node["private_key_path"] = ssl_config.private_key_path;
    node["ciphers"] = ssl_config.ciphers;
    node["dh_param_path"] = ssl_config.dh_param_path;
    node["protocols"] = ssl_config.protocols;
  }

  static bool decode(const YAML::Node& node, trpc::SslConfig& ssl_config) {
    if (node["enable"]) ssl_config.enable = node["enable"].as<bool>();
    if (node["cert_path"]) ssl_config.cert_path = node["cert_path"].as<std::string>();
    if (node["ca_cert_path"]) ssl_config.ca_cert_path = node["ca_cert_path"].as<std::string>();
    if (node["private_key_path"]) ssl_config.private_key_path = node["private_key_path"].as<std::string>();
    if (node["ciphers"]) ssl_config.ciphers = node["ciphers"].as<std::string>();
    if (node["dh_param_path"]) ssl_config.dh_param_path = node["dh_param_path"].as<std::string>();
    if (node["protocols"]) {
      for (size_t i = 0; i < node["protocols"].size(); ++i) {
        ssl_config.protocols.push_back(node["protocols"][i].as<std::string>());
      }
    }
    return true;
  }
};

template <>
struct convert<trpc::ServerSslConfig> {
  static YAML::Node encode(const trpc::ServerSslConfig& ssl_config) {
    YAML::Node node;
    convert<trpc::SslConfig>::encode(ssl_config, node);
    node["mutual_auth"] = ssl_config.mutual_auth;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::ServerSslConfig& ssl_config) {
    convert<trpc::SslConfig>::decode(node, ssl_config);
    if (node["mutual_auth"]) ssl_config.mutual_auth = node["mutual_auth"].as<bool>();
    return true;
  }
};

template <>
struct convert<trpc::ClientSslConfig> {
  static YAML::Node encode(const trpc::ClientSslConfig& ssl_config) {
    YAML::Node node;
    convert<trpc::SslConfig>::encode(ssl_config, node);
    node["sni_name"] = ssl_config.sni_name;
    node["insecure"] = ssl_config.insecure;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::ClientSslConfig& ssl_config) {
    convert<trpc::SslConfig>::decode(node, ssl_config);
    if (node["sni_name"]) ssl_config.sni_name = node["sni_name"].as<std::string>();
    if (node["insecure"]) ssl_config.insecure = node["insecure"].as<bool>();
    return true;
  }
};

}  // namespace YAML
