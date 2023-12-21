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

#include "trpc/transport/common/ssl/ssl.h"

#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif

#include <algorithm>

#include "trpc/transport/common/ssl/core.h"
#include "trpc/transport/common/ssl/errno.h"
#include "trpc/util/log/logging.h"

namespace trpc::ssl {

// ERR_error_string() generates a human-readable string representing the error code e, and places it at buf.
// buf must be at least 256 bytes long.
constexpr int kErrBufLen{256};

namespace {
// OpenSSL can generally be used safely in multi-thread applications provided that at least two callback functions
// are set:
// 1. the locking_function
// 2. the threadid_function
//
// Note that OpenSSL is not completely thread-safe, and unfortunately not all global resources have the necessary locks.
// Further, the thread-safety does not extend to things like multiple threads using the same SSL object at the same
// time.
//
// Reference to https://www.openssl.org/docs/man1.0.2/man3/threads.html.
std::mutex* ssl_mutexes = nullptr;

__attribute__((unused)) void SslLockCallbackFunction(int mode, int n, const char* file, int line) {
  if (mode & CRYPTO_LOCK) {
    ssl_mutexes[n].lock();
  } else {
    ssl_mutexes[n].unlock();
  }
}

__attribute__((unused)) uint64_t SslThreadIdCallbackFunction() { return static_cast<uint64_t>(pthread_self()); }

void InitSslMutexes() {
  int lock_nums = CRYPTO_num_locks();
  ssl_mutexes = new std::mutex[lock_nums];
  CRYPTO_set_id_callback(SslThreadIdCallbackFunction);
  CRYPTO_set_locking_callback(SslLockCallbackFunction);
}

void DestroySslMutexes() {
  if (!ssl_mutexes) return;
  CRYPTO_set_locking_callback(nullptr);
  delete[] ssl_mutexes;
  ssl_mutexes = nullptr;
}
}  // namespace

namespace {
// @brief Print state strings, information about alerts being handled and alerts being handled and error messages to the
// bio_err.
void SslInfoCallback(const SSL* s, int where, int ret) {
  // SSL_CTX_set_info_callback() sets the callback function, that can be used to obtain state information for SSL
  // objects created from ctx during connection setup and use.
  // The setting for ctx is overridden from the setting for a specific SSL object, if specified.
  // When callback is NULL, no callback function is used.
  // Reference to: https://www.openssl.org/docs/man1.1.1/man3/SSL_set_info_callback.html
  const char* str;
  int w = where & ~SSL_ST_MASK;
  if (w & SSL_ST_CONNECT)
    str = "SSL_connect";
  else if (w & SSL_ST_ACCEPT)
    str = "SSL_accept";
  else
    str = "undefined";

  if (where & SSL_CB_LOOP) {
    TRPC_LOG_DEBUG(str << ":" << SSL_state_string_long(s));
  } else if (where & SSL_CB_ALERT) {
    str = (where & SSL_CB_READ) ? "read" : "write";
    TRPC_LOG_DEBUG("SSLv3 alert " << str << ":" << SSL_alert_type_string_long(ret) << ":"
                                  << SSL_alert_desc_string_long(ret));
  } else if (where & SSL_CB_EXIT) {
    if (ret == 0) {
      TRPC_LOG_DEBUG(str << ":failed in " << SSL_state_string_long(s));
    } else if (ret < 0) {
      TRPC_LOG_DEBUG(str << ":error in " << SSL_state_string_long(s));
    }
  }
}
}  // namespace

namespace {
// @brief Load certificate and chain of certificate from certificate file which located on `cert_path`.
EVP_PKEY* LoadCertificateKey(std::string& err, const std::string& key_path) {
  BIO* bio = BIO_new_file(key_path.c_str(), "r");
  if (!bio) {
    err = "BIO_new_file() failed";
    return nullptr;
  }

  char* passwords = nullptr;
  pem_password_cb* cb = nullptr;
  // Read a private key from a BIO using a pass phrase callback
  EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, cb, passwords);

  if (!pkey) {
    err = "PEM_read_bio_PrivateKey() failed";
    BIO_free(bio);
    return nullptr;
  }

  return pkey;
}

// @brief  Load private key from key file which located on `key_path`.
X509* LoadCertificate(std::string& err, const std::string& cert_path, STACK_OF(X509) * *chain) {
  BIO* bio = BIO_new_file(cert_path.c_str(), "r");
  if (!bio) {
    err = "BIO_new_file() failed";
    return nullptr;
  }

  // Certificate itself.
  X509* x509 = PEM_read_bio_X509_AUX(bio, nullptr, nullptr, nullptr);
  if (!x509) {
    err = "PEM_read_bio_X509_AUX() failed.";
    BIO_free(bio);
    return nullptr;
  }

  // Rest of the certificate chain
  *chain = sk_X509_new_null();
  if (!*chain) {
    err = "sk_X509_new_null() failed";
    BIO_free(bio);
    X509_free(x509);
    return nullptr;
  }

  X509* temp = nullptr;
  u_long n = 0;

  for (;;) {
    temp = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    if (!temp) {
      n = ERR_peek_last_error();
      if (ERR_GET_LIB(n) == ERR_LIB_PEM && ERR_GET_REASON(n) == PEM_R_NO_START_LINE) {
        // End of file
        ERR_clear_error();
        break;
      }

      // Some real error
      err = "PEM_read_bio_X509() failed";
      BIO_free(bio);
      X509_free(x509);
      sk_X509_pop_free(*chain, X509_free);
      return nullptr;
    }

    // Push certificate into chain
    if (sk_X509_push(*chain, temp) == 0) {
      err = "sk_X509_push() failed";
      BIO_free(bio);
      X509_free(x509);
      sk_X509_pop_free(*chain, X509_free);
      return nullptr;
    }
  }

  BIO_free(bio);

  return x509;
}

// @brief Generates a 512-bits-key.
__attribute__((unused)) RSA* Rsa512KeyCallback(SSL* s, int is_export, int key_length) {
  static RSA* key = nullptr;

  if (key_length != 512) {
    return nullptr;
  }
#if (OPENSSL_VERSION_NUMBER < 0x10100003L && !defined OPENSSL_NO_DEPRECATED)
  if (!key) {
    key = RSA_generate_key(512, RSA_F4, nullptr, nullptr);
  }
#endif

  return key;
}
}  // namespace

bool InitOpenSsl() {
  // OpenSSL keeps an internal table of digest algorithms and ciphers.
  // It uses this table to lookup ciphers via functions such as EVP_get_cipher_byname().
  // In OpenSSL versions prior to 1.1.0 these functions initialised and de-initialised this table.
  // From OpenSSL 1.1.0 they are deprecated.
  // No explicit initialisation or de-initialisation is required.
  // Reference to: https://www.openssl.org/docs/man1.1.0/man3/OpenSSL_add_all_algorithms.html
  // OpenSSL Version Format.
  // Reference to: https://www.openssl.org/docs/man1.1.0/man3/OPENSSL_VERSION_NUMBER.html
#if OPENSSL_VERSION_NUMBER >= 0x10100003L
  if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, nullptr) == 0) {
    TRPC_LOG_ERROR("OPENSSL_init_ssl() failed");
    return false;
  }
  // OPENSSL_init_ssl() may leave errors in error queue while returning success.
  ERR_clear_error();
#else
  OPENSSL_config(nullptr);
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();
#endif

#ifndef SSL_OP_NO_COMPRESSION
  {
    // Disables gizip comparession in OpenSSL prior to 1.0.0 version, this saves about 522k per connection.
    STACK_OF(SSL_COMP)* ssl_comp_methods = nullptr;
    int n = 0;
    ssl_comp_methods = SSL_COMP_get_compression_methods();
    n = sk_SSL_COMP_num(ssl_comp_methods);
    while (n--) {
      (void)sk_SSL_COMP_pop(ssl_comp_methods);
    }
  }
#endif
  // Something other were initialized here.
  InitSslMutexes();
  return true;
}

void DestroyOpenSsl() {
#if OPENSSL_VERSION_NUMBER < 0x10100003L
  EVP_cleanup();
#ifndef OPENSSL_NO_ENGINE
  ENGINE_cleanup();
#endif

#endif
  DestroySslMutexes();
}

uint32_t ParseProtocols(const std::vector<std::string>& protocols) {
  uint32_t final_protocols = kSslUnknown;

  // Return the value of ssl protocol by parsing ssl protocol string
  auto SslProtocolParseFunction = [](const std::string& protocol) {
    if (kSslSslV2Text == protocol)
      return kSslSslV2;
    else if (kSslSslV3Text == protocol)
      return kSslSslV3;
    else if (kSslTlsV1Text == protocol)
      return kSslTlsV1;
    else if (kSslTlsV11Text == protocol)
      return kSslTlsV11;
    else if (kSslTlsV12Text == protocol)
      return kSslTlsV12;
    else if (kSslTlsV13Text == protocol)
      return kSslTlsV13;
    else
      return kSslUnknown;
  };

  for (auto& protocol : protocols) {
    final_protocols |= SslProtocolParseFunction(protocol);
  }

  // Return parsed protocols if parsed one or more well.
  if (final_protocols) return final_protocols;

  // Default protocols: TLSv1.1 | TLSv1.2
  uint32_t default_protocols = kSslTlsV11 | kSslTlsV12;

  return default_protocols;
}

const std::string& GetDefaultCiphers() {
  static std::string ciphers = R"()"
                               R"(ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256)"
                               R"(:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384)"
                               R"(:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256)"
                               R"(:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA)"
                               R"(:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA)"
                               R"(:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA)"
                               R"(:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA)"
                               R"(:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-SHA256)"
                               R"(:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK)";
  return ciphers;
}

SslContext::~SslContext() {
  if (ssl_ctx_) {
    SSL_CTX_free(ssl_ctx_);
    ssl_ctx_ = nullptr;
  }
}

bool SslContext::Init(const ServerSslOptions& ssl_options) {
  // New ssl context
  if (!SetSslCtx(ssl_options.protocols)) return false;

  // Load certificate and key
  if (!SetCertificate(ssl_options.default_cert.cert_path, ssl_options.default_cert.private_key_path)) return false;

  // Set cipher suite
  uint32_t prefer_server_ciphers = 1;
  if (!SetCiphers(ssl_options.ciphers, prefer_server_ciphers)) return false;

  // Set dh_param
  if (!ssl_options.dh_param_path.empty()) {
    if (!SetDhParam(ssl_options.dh_param_path)) return false;
  }

  return this->SetSslVerifyPeerOptions(ssl_options.verify_peer_options.ca_cert_path,
                                       ssl_options.verify_peer_options.verify_depth, !ssl_options.enable_verify_peer);
}

bool SslContext::Init(const ClientSslOptions& ssl_options) {
  // New ssl context
  if (!SetSslCtx(ssl_options.protocols)) return false;

  if (!ssl_options.client_cert.private_key_path.empty() && !ssl_options.client_cert.cert_path.empty()) {
    if (!SetCertificate(ssl_options.client_cert.cert_path, ssl_options.client_cert.private_key_path)) return false;
  }

  // Set cipher suite
  uint32_t prefer_server_ciphers = 1;
  if (!SetCiphers(ssl_options.ciphers, prefer_server_ciphers)) return false;

  // Set dh_param
  if (!ssl_options.dh_param_path.empty()) {
    if (!SetDhParam(ssl_options.dh_param_path)) return false;
  }

  return this->SetSslVerifyPeerOptions(ssl_options.verify_peer_options.ca_cert_path,
                                       ssl_options.verify_peer_options.verify_depth, ssl_options.insecure);
}

bool SslContext::SetSslCtx(uint32_t protocols) {
  // Some options settings reference to nginx source code.

  if (ssl_ctx_) return true;

  const SSL_METHOD* method = SSLv23_method();
  ssl_ctx_ = SSL_CTX_new(method);
  if (!ssl_ctx_) {
    TRPC_LOG_ERROR("SSL_CTX_new() failed");
    return false;
  }

  // <-- client side options
#ifdef SSL_OP_MICROSOFT_SESS_ID_BUG
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_MICROSOFT_SESS_ID_BUG);
#endif

#ifdef SSL_OP_NETSCAPE_CHALLENGE_BUG
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_NETSCAPE_CHALLENGE_BUG);
#endif
  // --> client side options

  // <-- Server side options
#ifdef SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG);
#endif

#ifdef SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER);
#endif

#ifdef SSL_OP_MSIE_SSLV2_RSA_PADDING
  // This option allow a potential SSL 2.0 rollback (CAN-2005-2969)
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_MSIE_SSLV2_RSA_PADDING);
#endif

#ifdef SSL_OP_SSLEAY_080_CLIENT_DH_BUG
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_SSLEAY_080_CLIENT_DH_BUG);
#endif

#ifdef SSL_OP_TLS_D5_BUG
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_TLS_D5_BUG);
#endif

#ifdef SSL_OP_TLS_BLOCK_PADDING_BUG
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_TLS_BLOCK_PADDING_BUG);
#endif

#ifdef SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
#endif

  SSL_CTX_set_options(ssl_ctx_, SSL_OP_SINGLE_DH_USE);

#if OPENSSL_VERSION_NUMBER >= 0x009080dfL
  // only in 0.9.8m+
  SSL_CTX_clear_options(ssl_ctx_, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1);

#endif

  // Set protocol options.
  SetSslCtxProtocols(protocols);

#ifdef SSL_CTX_set_min_proto_version
  SSL_CTX_set_min_proto_version(ssl_ctx_, 0);
  SSL_CTX_set_max_proto_version(ssl_ctx_, TLS1_2_VERSION);
#endif

#ifdef TLS1_3_VERSION
  SSL_CTX_set_min_proto_version(ssl_ctx_, 0);
  SSL_CTX_set_max_proto_version(ssl_ctx_, TLS1_3_VERSION);
#endif

#ifdef SSL_OP_NO_COMPRESSION
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_COMPRESSION);
#endif

#ifdef SSL_OP_NO_ANTI_REPLAY
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_ANTI_REPLAY);
#endif

#ifdef SSL_OP_NO_CLIENT_RENEGOTIATION
  SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_CLIENT_RENEGOTIATION);
#endif

#ifdef SSL_MODE_RELEASE_BUFFERS
  SSL_CTX_set_mode(ssl_ctx_, SSL_MODE_RELEASE_BUFFERS);
#endif

#ifdef SSL_MODE_NO_AUTO_CHAIN
  SSL_CTX_set_mode(ssl_ctx_, SSL_MODE_NO_AUTO_CHAIN);
#endif

  SSL_CTX_set_read_ahead(ssl_ctx_, 1);

  SSL_CTX_set_info_callback(ssl_ctx_, SslInfoCallback);
  // --> Server side options

#ifdef SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER
  // fix: error:1409F07F:SSL routines:SSL3_WRITE_PENDING: bad write retry
  SSL_CTX_set_mode(ssl_ctx_, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#endif

  return true;
}

bool SslContext::SetCertificate(const std::string& cert_path, const std::string& key_path) {
  if (!ssl_ctx_) {
    TRPC_LOG_ERROR("ssl_ctx_ is nullptr");
    return false;
  }

  std::string err;
  X509* x509 = nullptr;
  EVP_PKEY* pkey = nullptr;
  STACK_OF(X509)* chain = nullptr;

  x509 = LoadCertificate(err, cert_path, &chain);
  if (!x509) {
    TRPC_LOG_ERROR("LoadCertificate() failed: " << err);
    return false;
  }

  // Set certificate itself
  if (SSL_CTX_use_certificate(ssl_ctx_, x509) == 0) {
    TRPC_LOG_ERROR("SSL_CTX_use_certificate() failed");
    X509_free(x509);
    sk_X509_pop_free(chain, X509_free);
    return false;
  }

  // Set certificate-chain
#ifdef SSL_CTX_set0_chain
  if (SSL_CTX_set0_chain(ssl_ctx_, chain) == 0) {
    TRPC_LOG_ERROR("SSL_CTX_set0_chain() failed");
    sk_X509_pop_free(chain, X509_free);
    return false;
  }
#else
  {
    // SSL_CTX_set0_chain() is only available in OpenSSL 1.0.2+
    int n = sk_X509_num(chain);
    while (n--) {
      x509 = sk_X509_shift(chain);

      if (SSL_CTX_add_extra_chain_cert(ssl_ctx_, x509) == 0) {
        TRPC_LOG_ERROR("SSL_CTX_add_extra_chain_cert() failed");
        sk_X509_pop_free(chain, X509_free);

        return false;
      }
    }
    sk_X509_free(chain);
  }
#endif

  pkey = LoadCertificateKey(err, key_path);

  if (!pkey) {
    TRPC_LOG_ERROR("LoadCertificateKey() failed");
    return false;
  }

  if (SSL_CTX_use_PrivateKey(ssl_ctx_, pkey) == 0) {
    TRPC_LOG_ERROR("SSL_CTX_use_PrivateKey() failed");
    EVP_PKEY_free(pkey);
    return false;
  }

  EVP_PKEY_free(pkey);
  return true;
}

bool SslContext::SetCiphers(const std::string& ciphers, const uint32_t prefer_server_ciphers) {
  if (!ssl_ctx_) {
    TRPC_LOG_ERROR("ssl_ctx_ is nullptr");
    return false;
  }

  if (SSL_CTX_set_cipher_list(ssl_ctx_, ciphers.c_str()) == 0) {
    TRPC_LOG_ERROR("SSL_CTX_set_cipher_list() failed");
    return false;
  }

  if (prefer_server_ciphers) {
    SSL_CTX_set_options(ssl_ctx_, SSL_OP_CIPHER_SERVER_PREFERENCE);
  }

#if OPENSSL_VERSION_NUMBER < 0x10100001L
  SSL_CTX_set_tmp_rsa_callback(ssl_ctx_, Rsa512KeyCallback);
#endif

  return true;
}

bool SslContext::SetDhParam(const std::string& dh_param_path) {
  if (!ssl_ctx_) {
    TRPC_LOG_ERROR("ssl_ctx_ is nullptr");
    return false;
  }

  BIO* bio = BIO_new_file(dh_param_path.c_str(), "r");
  if (!bio) {
    TRPC_LOG_ERROR("BIO_new_file() failed");
    return false;
  }

  DH* dh = PEM_read_bio_DHparams(bio, nullptr, nullptr, nullptr);
  if (!dh) {
    TRPC_LOG_ERROR("PEM_read_bio_DHparams() failed");
    BIO_free(bio);
    return false;
  }

  // SSL_CTX_set_tmp_dh() sets DH parameters to be used to be dh.
  // The key is inherited by all ssl objects created from ctx.
  // Reference to: https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_set_tmp_dh.html
  SSL_CTX_set_tmp_dh(ssl_ctx_, dh);
  DH_free(dh);
  BIO_free(bio);

  return true;
}

bool SslContext::SetClientCertificate(const std::string& cert_path, int depth) {
  if (!ssl_ctx_) {
    TRPC_LOG_ERROR("ssl_ctx_ is nullptr");
    return false;
  }

  // SSL_CTX_set_verify() sets the verification flags for ctx to be mode
  // -- and specifies the verify_callback function to be used. If no callback
  // -- function shall be specified, the NULL pointer can be used for verify_callback.
  //
  // SSL_CTX_set_verify_depth() sets the maximum depth for the certificate chain
  // -- verification that shall be allowed for ctx.
  // Reference to: https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_verify.html
  // FIXME: implement cert_verify_callback
  SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER, nullptr);
  SSL_CTX_set_verify_depth(ssl_ctx_, depth);

  // SSL_CTX_load_verify_locations() specifies the locations for ctx,
  // -- at which CA certificates for verification purposes are located.
  // The certificates available via CAfile and CApath are trusted.
  if (SSL_CTX_load_verify_locations(ssl_ctx_, cert_path.c_str(), nullptr) == 0) {
    TRPC_LOG_ERROR("SSL_CTX_load_verify_locations() failed, cert path:" << cert_path);
    return false;
  }

  // SSL_CTX_load_verify_locations() may leave errors in error queue while returning success.
  ERR_clear_error();

  STACK_OF(X509_NAME)* list = SSL_load_client_CA_file(cert_path.c_str());
  if (!list) {
    TRPC_LOG_ERROR("SSL_load_client_CA_file() failed, cert path:" << cert_path);
    return false;
  }

  // SSL_CTX_set_client_CA_list() sets the list of CAs sent to the client when requesting a client certificate for ctx.
  // Ownership of list is transferred to ctx and it should not be freed by the caller.
  // Reference to: https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_client_CA_list.html
  SSL_CTX_set_client_CA_list(ssl_ctx_, list);

  return true;
}

bool SslContext::SetSslVerifyPeerOptions(const std::string& cert_path, int verify_depth, bool verify_none) {
  if (!ssl_ctx_) {
    TRPC_LOG_ERROR("ssl_ctx_ is nullptr");
    return false;
  }
  // FIXME: implement cert_verify_callback
  if (verify_none) {
    SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_NONE, nullptr);
  } else {
    SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_verify_depth(ssl_ctx_, verify_depth);
    if (!cert_path.empty() && SSL_CTX_load_verify_locations(ssl_ctx_, cert_path.c_str(), nullptr) == 0) {
      TRPC_LOG_ERROR("SSL_CTX_load_verify_locations() failed, cert path:" << cert_path);
      return false;
    }
  }
  return true;
}

SslPtr SslContext::NewSsl() {
  if (ssl_ctx_) {
    SSL* ssl = SSL_new(ssl_ctx_);
    if (ssl) return MakeRefCounted<Ssl>(ssl);
  }
  return nullptr;
}

void SslContext::SetSslCtxProtocols(const uint32_t protocols) {
  if (!(protocols & kSslSslV2)) SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_SSLv2);

  if (!(protocols & kSslSslV3)) SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_SSLv3);

  if (!(protocols & kSslTlsV1)) SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_TLSv1);

#ifdef SSL_OP_NO_TLSv1_1
  SSL_CTX_clear_options(ssl_ctx_, SSL_OP_NO_TLSv1_1);
  if (!(protocols & kSslTlsV11)) SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_TLSv1_1);
#endif

#ifdef SSL_OP_NO_TLSv1_2
  SSL_CTX_clear_options(ssl_ctx_, SSL_OP_NO_TLSv1_2);
  if (!(protocols & kSslTlsV12)) SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_TLSv1_2);
#endif

#ifdef SSL_OP_NO_TLSv1_3
  SSL_CTX_clear_options(ssl_ctx_, SSL_OP_NO_TLSv1_3);
  if (!(protocols & kSslTlsV13)) SSL_CTX_set_options(ssl_ctx_, SSL_OP_NO_TLSv1_3);
#endif
}

Ssl::~Ssl() {
  if (ssl_) {
    SSL_shutdown(ssl_);
    SSL_free(ssl_);
    ssl_ = nullptr;
  }
}

void Ssl::SetAcceptState() { SSL_set_accept_state(ssl_); }

void Ssl::SetConnectState() { SSL_set_connect_state(ssl_); }

int Ssl::DoHandshake() {
  // SSL_do_handshake() will wait for a SSL/TLS handshake to take place.
  // If the connection is in client mode, the handshake will be started.
  // The handshake routines may have to be explicitly set in advance
  // -- using either SSL_set_connect_state(3) or SSL_set_accept_state(3).
  // Reference to: https://www.openssl.org/docs/man1.1.1/man3/SSL_do_handshake.html

  int n = SSL_do_handshake(ssl_);

  // handshake successfully completed
  if (n == 1) return kOk;

  int ssl_err = SSL_get_error(ssl_, n);

  if (ssl_err == SSL_ERROR_WANT_READ) {
    TRPC_LOG_DEBUG("SSL_do_handshake() failed, ssl error: SSL_ERROR_WANT_READ");
    return kWantRead;
  }

  if (ssl_err == SSL_ERROR_WANT_WRITE) {
    TRPC_LOG_DEBUG("SSL_do_handshake() failed, ssl error: SSL_ERROR_WANT_WRITE");
    return kWantWrite;
  }

  trpc_err_t err = (ssl_err == SSL_ERROR_SYSCALL) ? trpc_errno() : 0;

  if (ssl_err == SSL_ERROR_ZERO_RETURN || ERR_peek_error() == 0) {
    TRPC_LOG_DEBUG("peer closed connection in SSL handshake, ssl error:" << ssl_err);
    return kError;
  }

  char err_str[kErrBufLen]{'\0'};
  ERR_error_string_n(ERR_peek_error(), err_str, kErrBufLen);
  TRPC_LOG_DEBUG("SSL_do_handshake(), ssl error:" << ssl_err << ", errno:" << err
                                                  << ", peek error string: " << err_str);

  return kError;
}

ssize_t Ssl::Recv(char* buf, size_t size) {
  int n = 0, read_bytes = 0;
  int ssl_err;
  trpc_err_t err;

  // SSl_read() may return data in parts, so try to read until SSL_read() would return no data.
  for (;;) {
    n = SSL_read(ssl_, buf, size);

    // Read n bytes succeed
    if (n > 0) {
      read_bytes += n;
      size -= n;
      if (size == 0) {
        return read_bytes;
      }
      buf += n;
      continue;
    }

    // Return read bytes count if read some
    if (read_bytes) {
      return read_bytes;
    }

    // Read bytes failed, try to parse errors occurred.
    ssl_err = SSL_get_error(ssl_, n);
    err = (ssl_err == SSL_ERROR_SYSCALL) ? trpc_errno() : 0;

    if (ssl_err == SSL_ERROR_WANT_WRITE) {
      return kWantWrite;
    }

    if (ssl_err == SSL_ERROR_WANT_READ) {
      return kWantRead;
    }

    if (ssl_err == SSL_ERROR_ZERO_RETURN || ERR_peek_error() == 0) {
      TRPC_LOG_DEBUG("peer closed connection in SSL_read(), ssl error:" << ssl_err);
      return kError;
    }

    char err_str[kErrBufLen]{'\0'};
    ERR_error_string_n(ERR_peek_error(), err_str, kErrBufLen);
    TRPC_LOG_DEBUG("SSL_read(), ssl error:" << ssl_err << ", errno:" << err << ", peek error string: " << err_str);

    return kError;
  }

  return read_bytes;
}

ssize_t Ssl::Send(const char* buf, size_t size) {
  int n = 0;
  int ssl_err;
  trpc_err_t err;

  n = SSL_write(ssl_, buf, size);

  // Write n bytes succeed
  if (n > 0) {
    return n;
  }

  // Write bytes failed, try to parse errors occurred
  ssl_err = SSL_get_error(ssl_, n);
  err = (ssl_err == SSL_ERROR_SYSCALL) ? trpc_errno() : 0;

  if (ssl_err == SSL_ERROR_WANT_WRITE) {
    return kWantWrite;
  }

  if (ssl_err == SSL_ERROR_WANT_READ) {
    return kWantRead;
  }

  if (ssl_err == SSL_ERROR_ZERO_RETURN || ERR_peek_error() == 0) {
    TRPC_LOG_DEBUG("peer closed connection in SSL_write(), ssl error:" << ssl_err);
    return kError;
  }

  char err_str[kErrBufLen]{'\0'};
  ERR_error_string_n(ERR_peek_error(), err_str, kErrBufLen);
  TRPC_LOG_DEBUG("SSL_write(), ssl error:" << ssl_err << ", errno:" << err << ", peek error string: " << err_str);

  return kError;
}

ssize_t Ssl::Writev(const struct iovec* iov, int iovcnt) { return SendOnce(iov, iovcnt); }

bool Ssl::SetFd(const int fd) {
  if (SSL_set_fd(ssl_, fd) == 1) return true;
  // Error occurred
  TRPC_LOG_ERROR("SSL_set_fd() failed");
  return false;
}

int Ssl::GetFd() const {
  int fd = SSL_get_fd(ssl_);
  if (fd >= 0) return fd;
  // Error occurred
  TRPC_LOG_ERROR("SSL_get_fd() failed");
  return kError;
}

int Ssl::Shutdown() {
  if (SSL_in_init(ssl_)) {
    // OpenSSL 1.0.2f complains if SSL_Shutdown() is called during a SSL handshake, while previous version always
    // return 0.
    // Avoid calling SSL_shutdown() if handshake wasn't completed.
    SSL_free(ssl_);
    ssl_ = nullptr;
    return kOk;
  }

  int n = SSL_shutdown(ssl_);
  int ssl_err = 0;

  // before 0.9.8m SSL_shutdown() return 0 instead of -1 on errors.
  if (n != 1 && ERR_peek_error()) {
    ssl_err = SSL_get_error(ssl_, n);
    TRPC_LOG_ERROR("SSL_get_error(): " << ssl_err);
  }

  if (n == 1 || ssl_err == 0 || ssl_err == SSL_ERROR_ZERO_RETURN) {
    SSL_free(ssl_);
    ssl_ = nullptr;
    return kOk;
  }

  if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE) {
    return kAgain;
  }

  trpc_err_t err = (ssl_err == SSL_ERROR_SYSCALL) ? trpc_errno() : 0;
  TRPC_LOG_ERROR("SSL_shutdown() failed, errno: " << err);

  SSL_free(ssl_);
  ssl_ = nullptr;

  return kError;
}

bool Ssl::SetTlsExtensionServerName(const std::string& server_name) {
  if (SSL_set_tlsext_host_name(ssl_, server_name.c_str()) == 1) return true;
  // Error occurred
  TRPC_LOG_ERROR("SSL_set_tlsext_host_name() failed");
  return false;
}

ssize_t Ssl::SendOnce(const struct iovec* iov, int iovcnt) {
  // Reference to https://code.woboq.org/userspace/glibc/sysdeps/posix/writev.c.html
  static constexpr std::size_t kMaxLocalSize = 128 * 1024;
  thread_local auto local_buffer = std::make_unique<char[]>(kMaxLocalSize);
  char* buffer = local_buffer.get();
  char* bp;
  size_t bytes, to_copy;
  int i;

  // Find the total number of bytes to be written.
  bytes = 0;
  for (i = 0; i < iovcnt; ++i) bytes += iov[i].iov_len;

  // Allocate dynamically a temporary buffer to hold the data.
  if (bytes > kMaxLocalSize) {
    buffer = reinterpret_cast<char*>(malloc(bytes));
    if (!buffer) return kError;
  }

  // Copy the data into BUFFER.
  to_copy = bytes;
  bp = buffer;
  for (i = 0; i < iovcnt; ++i) {
    size_t copy = std::min(iov[i].iov_len, to_copy);
    memcpy(reinterpret_cast<void*>(bp), reinterpret_cast<void*>(iov[i].iov_base), copy);
    bp += copy;
    to_copy -= copy;
    if (to_copy == 0) break;
  }

  ssize_t send_bytes = this->Send(buffer, bytes);

  if (bytes > kMaxLocalSize) {
    free(buffer);
  }

  return send_bytes;
}

}  // namespace trpc::ssl
#endif
