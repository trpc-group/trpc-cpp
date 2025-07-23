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

#include <string>
#include <vector>

namespace trpc {

/// Contains certificate, ciphers, protocols of TLS.
struct SslConfig {
  /// To indicate enable ssl or not(true: enable, false: disable)
  bool enable{false};

  /// Path of certificate, e.g., /path/to/../xx_cert.pem
  std::string cert_path;

  /// Path of CA certificate, e.g., /path/to/../xx_cert.pem
  /// for verifying the peer's certificate
  std::string ca_cert_path;

  /// Path of private key, e.g., /path/to/../xx_key.pem
  std::string private_key_path;

  /// Cipher suite of SSL/TLS
  std::string ciphers;

  /// Path of dh_param, e.g., /path/to/../xx_dh_param.dhparam
  std::string dh_param_path;

  /// Protocols of SSL/TLS, e.g, ["SSLv3", "TSLv1", ... , "TLSv1.2"]
  std::vector<std::string> protocols;

  /// @brief Display content of struct
  std::string ToString() const;
};

/// @brief content of ssl_cfg.yaml on client side like below:
/// ...
/// service:
///     - name: xx_name
///       ...
///       ssl:
///           enable: { true , false }
///           sni_name: xx.com
///           ca_cert_path: xx_ca_cert.pem
///           private_key_path: xx_key.pem
///           ciphers: xx_cipher_suite
///           dh_param_path: xx_dh_param.dhparam
///           insecure: { true, false }
///           protocols:
///               - SSLv2
///               - SSLv3
///               - TLSv1
///               - TLSv1.1
///               - TLSv1.2
///               - TLSv1.3
/// ...
///
struct ClientSslConfig : public SslConfig {
  /// This option will be set into SNI extension filed during SSL handshaking.
  /// Default: "".
  std::string sni_name;

  /// If true, allow connections to SSL sites without certs(ture: enable, false: disable).
  bool insecure{false};

  /// @brief Display content of struct.
  void Display() const;
};

/// @brief content of ssl_cfg.yaml on server side like below:
/// ...
/// service:
///     - name: xx_name
///       ...
///       ssl:
///           enable: { true , false }
///           mutual_auth: { true, false }
///           cert_path: xx_cert.pem
///           ca_cert_path: xx_cert.pem
///           private_key_path: xx_key.pem
///           ciphers: xx_cipher_suite
///           dh_param_path: xx_dh_param.dhparam
///           protocols:
///               - SSLv2
///               - SSLv3
///               - TLSv1
///               - TLSv1.1
///               - TLSv1.2
///               - TLSv1.3
/// ...
///
struct ServerSslConfig : public SslConfig {
  /// If true, enable mutual SSL/TLS authentication.
  bool mutual_auth{false};

  /// @brief Display content of struct.
  void Display() const;
};

}  // namespace trpc
