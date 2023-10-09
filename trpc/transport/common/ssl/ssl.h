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

#ifdef TRPC_BUILD_INCLUDE_SSL
#pragma once

#include <openssl/ssl.h>
#include <sys/uio.h>

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "trpc/transport/common/ssl/core.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::ssl {
// Integer value of protocols in SSL/TLS.
constexpr uint32_t kSslUnknown = 0x0000;
constexpr uint32_t kSslSslV2 = 0x0002;
constexpr uint32_t kSslSslV3 = 0x0004;
constexpr uint32_t kSslTlsV1 = 0x0008;
constexpr uint32_t kSslTlsV11 = 0x0010;
constexpr uint32_t kSslTlsV12 = 0x0020;
constexpr uint32_t kSslTlsV13 = 0x0040;

// Text value of protocols in SSL/TLS.
constexpr char kSslSslV2Text[] = "SSLv2";
constexpr char kSslSslV3Text[] = "SSLv3";
constexpr char kSslTlsV1Text[] = "TLSv1";
constexpr char kSslTlsV11Text[] = "TLSv1.1";
constexpr char kSslTlsV12Text[] = "TLSv1.2";
constexpr char kSslTlsV13Text[] = "TLSv1.3";

/// @brief Init library of OpenSSL.
bool InitOpenSsl();

/// @brief Do some cleanup job about library of OpenSSL.
void DestroyOpenSsl();

/// @brief Convert string-protocols to integer-protocols.
uint32_t ParseProtocols(const std::vector<std::string>& protocols);

/// @brief  Return default ssl cipher
const std::string& GetDefaultCiphers();

/// @brief Options for certificate.
struct CertificateOptions {
  // Path of certificate(format: PEM, e.g., /path/to/xx_cert.pem)
  std::string cert_path;

  // Path of private key(format: PEM, e.g., /path/to/xx_key.pem)
  std::string private_key_path;
};

/// @brief Options for verification.
struct VerifyPeerOptions {
  // Set maximum depth of the certificate chain for verification.
  // The depth count is "level 0:peer certificate", "level 1: CA certificate",
  // -- "level 2: higher level CA certificate", and so on.
  // Setting the maximum depth to 2 allows the levels 0, 1, 2 and 3
  // -- (0 being the end-entity and 3 the trust-anchor).
  // Default: 2.
  int verify_depth{2};

  // Set the trusted CA file to verify the peer's certificate.
  // If empty, use the default CA files on system.
  // Default: "".
  std::string ca_cert_path;
};

/// @brief Basic SSL options.
struct SslOptions {
  // Cipher suite.
  // e.g., DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT.
  std::string ciphers;

  // Protocols of SSL/TLS
  // e.g., kSslTlsV1|kSslTlsV11|kSslTlsV12.
  uint32_t protocols{0};

  // Path of DH-Parameter(e.g., /path/to/xx.dhparam).
  // Reserved, not currently used.
  std::string dh_param_path;

  // Options about verifying peer.
  // Reserved, not currently used.
  VerifyPeerOptions verify_peer_options;
};

/// @brief Options for client SSL.
struct ClientSslOptions : public SslOptions {
  // This option will be set into SNI extension filed during SSL handshaking.
  // Default: "".
  std::string sni_name;

  // Certificate used for client authentication.
  // Reserved, not currently used.
  CertificateOptions client_cert;

  // Options about verifying peer.
  // Reserved, not currently used.
  VerifyPeerOptions verify_peer_options;

  // Allow connections to SSL sites without certs.
  bool insecure{false};
};

/// @brief Options for server SSL.
struct ServerSslOptions : public SslOptions {
  // Path of certificate(format: PEM, e.g., /path/to/xx_cert.pem)
  CertificateOptions default_cert;

  // Verify client or not.
  // Reserved, not currently used.
  bool enable_verify_peer{false};
};

/// @brief A wrapper of SSL structure which is needed to hold the data for a TLS/SSL connection. The new structure
/// inherits the settings of the underlying context ctx: connection method, options, verification settings,
/// timeout settings.
class Ssl : public RefCounted<Ssl> {
 public:
  explicit Ssl(SSL* ssl) : ssl_(ssl) {}

  ~Ssl();

  /// @brief Works in server mode.
  void SetAcceptState();

  /// @brief Works in client mode.
  void SetConnectState();

  /// @brief Sets socket fd to SSL.
  bool SetFd(const int fd);

  /// @brief Gets socket fd linked to SSL
  int GetFd() const;

  /// @brief Waits for a SSL/TLS handshake to take place.
  ///
  /// @return Returns 0 on success, error occurred otherwise.
  /// 0: The TLS/SSL handshake was successfully completed.
  /// -1: The TLS/SSL handshake was not successful because a fatal error occurred either at the protocol level or
  /// a connection failure occurred.
  int DoHandshake();

  /// @brief Try to read |size| bytes from the specified ssl into the buffer |buf|.
  ///
  /// @return The following values can occur:
  /// > 0, The read operation was successful. The return value is the number of bytes actually read from the TLS/SSL
  /// connection.
  /// <= 0, The read operation was not successful, because either the connection was closed, an error occurred or
  /// action must be taken by the calling process.
  ssize_t Recv(char* buf, size_t size);

  /// @brief Writes num bytes from the buffer |buf| into the specified ssl connection.
  ///
  /// @brief The following values can occur:
  /// > 0: The write operation was successful, the return value is the number of bytes actually written to the TLS/SSL
  /// connection.
  /// <= 0: The write operation was not successful, because either the connection was closed, an error occurred or
  /// action must be taken by the calling process.
  ssize_t Send(const char* buf, size_t size);

  ///  @brief Acts as `Writev` function.
  ssize_t Writev(const struct iovec* iov, int iovcnt);

  /// @brief Shutdown a ssl session, then free it.
  int Shutdown();

  /// @brief Sets `server_name` for SNI which will set the server name indication ClientHello extension to contain
  /// the value `server_name`.
  bool SetTlsExtensionServerName(const std::string& server_name);

 private:
  ssize_t SendOnce(const struct iovec* iov, int iovcnt);

 private:
  SSL* ssl_{nullptr};
};
using SslPtr = RefPtr<Ssl>;

/// @brief A wrapper of SSL_CTX object which holds various configuration and data relevant to SSL/TLS or DTLS
/// session establishment.
class SslContext : public RefCounted<SslContext> {
 public:
  ~SslContext();

  /// @brief Creates a ssl session.
  SslPtr NewSsl();

  /// @brief Inits SSL Context for server mode.
  bool Init(const ServerSslOptions& ssl_options);

  /// @brief Inits SSL Context for client mode.
  bool Init(const ClientSslOptions& ssl_options);

 private:
  // @brief Sets a SSL context.
  bool SetSslCtx(const uint32_t protocols);

  // @brief Loads server-side's certificate and key.
  bool SetCertificate(const std::string& cert_path, const std::string& key_path);

  // @brief Sets preferred cipher suite.
  // Sets parameter `prefer_server_ciphers` to `1` if you prefer to use server-side's ciphers.
  bool SetCiphers(const std::string& ciphers, const uint32_t prefer_server_ciphers);

  // @brief Sets initialized DH-Parameter.
  bool SetDhParam(const std::string& dh_param_path);

  // @brief Sets client-side's CA-Certificate if wanted to do client-authentication.
  // Parameter `depth` indicates length of certificate-chain.
  bool SetClientCertificate(const std::string& cert_path, int depth);

  // @brief Sets `verify_callback` function for SSL
  bool SetSslVerifyPeerOptions(const std::string& cert_path, int verify_depth, bool verify_none);

  // @brief Sets SSL context with protocols.
  void SetSslCtxProtocols(const uint32_t protocols);

 private:
  // `ssl_ctx_` stores parsed well certificate, key and cipher suite, protocols of SSL/TLS.
  // It was used to create a ssl connection.
  SSL_CTX* ssl_ctx_{nullptr};
};
using SslContextPtr = RefPtr<SslContext>;

}  // namespace trpc::ssl
#endif
