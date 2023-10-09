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

#include <sstream>
#include <string>
#include <vector>

#include "trpc/util/http/common.h"
#include "trpc/util/http/field_map.h"
#include "trpc/util/http/method.h"
#include "trpc/util/http/status.h"
#include "trpc/util/http/url.h"

namespace trpc {

/// @brief Namespace of http sub-module.
namespace http {

/// @brief A map represents the key-value pairs in an HTTP header or HTTP trailer. The key is case-insensitive.
///
/// A field name labels the corresponding field value as having the semantic defined by that name. Field name is
/// case-insensitive.
///
/// A field value dose not include leading or tailing whitespace. Field values are usually constrained to the
/// range of US-ASCII.
///
/// RFC : https://www.rfc-editor.org/rfc/rfc9110.html#name-fields
using HeaderFieldMap = detail::FieldMap<detail::CaseInsensitiveLess>;

/// @brief  A map represents the key-value pairs in an HTTP header.
using HeaderPairs = HeaderFieldMap;

/// @brief A map represents the key-value pairs in an HTTP trailer.
using TrailerPairs = HeaderFieldMap;

/// @brief Represents the header of HTTP protocol message.
/// It contains:
/// - Request line: request method, request  URI, version.
/// - Status line: version, status, reason phrase.
/// - Header fields: key-value pairs (Key is case-insensitive).
///
/// About status: we usually use "status" only. It also contains reason phrase if user wants to set self-defined
/// status and reason phrase. e.g. "600 user-defined-text"
///
class Header {
 public:
  /// @brief Gets the HTTP method name ("GET", "POST", "PUT", etc.).
  /// Default method is "GET".
  const std::string& GetMethod() const {
    return method_type_ != MethodType::UNKNOWN ? MethodName(method_type_) : method_name_;
  }

  /// @brief Sets the HTTP method name ("GET", "POST", "PUT", etc.).
  void SetMethod(std::string_view name) {
    method_type_ = MethodNameToMethodType(name);
    if (method_type_ == MethodType::UNKNOWN) {
      method_name_ = name;
    }
  }

  /// @brief Gets the HTTP method type (GET, POST, PUT, etc.).
  /// Default method is GET.
  MethodType GetMethodType() const { return method_type_; }

  /// @brief Sets the HTTP method type (GET, POST, PUT, etc.).
  void SetMethodType(MethodType type) { method_type_ = type; }

  /// @brief Gets the request URI.
  const std::string& GetUrl() const { return request_uri_; }
  std::string* GetMutableUrl() { return &request_uri_; }

  /// @brief Sets the request URI.
  void SetUrl(std::string url) { request_uri_ = std::move(url); }

  /// @brief Gets the path field in request URI for dispatching request to HTTP handler.
  std::string GetRouteUrl() const { return std::string{GetRouteUrlView()}; }
  std::string_view GetRouteUrlView() const {
    return std::string_view(request_uri_).substr(0, std::min(request_uri_.find('?'), request_uri_.length()));
  }

  /// @brief Gets the version of HTTP protocol.
  /// The default version is 1.1.
  const std::string& GetVersion() const { return version_; }

  /// @brief Sets the version of HTTP protocol.
  void SetVersion(std::string version) {
    if (version == "1.1") {
      SetMajorMinorVersion(1, 1);
    } else if (version == "1.0") {
      SetMajorMinorVersion(1, 0);
    } else if (version == "2.0") {
      SetMajorMinorVersion(2, 0);
    } else if (version == "0.9") {
      SetMajorMinorVersion(0, 9);
    } else {
      return;
    }
    version_ = std::move(version);
  }
  /// @brief Returns major number.
  int GetVersionMajor() const { return major_; }

  /// @brief Returns minor number.
  int GetVersionMinor() const { return minor_; }

  /// @brief Reports whether the version less than HTTP/1.1.
  bool LessThanHttp1_1() const { return ((major_ << 10) + minor_) <= (1 << 10); }

  /// @brief Reports whether the major version equals HTTP/2.0.
  bool IsHttp2() const { return major_ == 2; }

  /// @brief Returns version string with prefix-"HTTP/", e.g. "HTTP/1.1".
  std::string FullVersion() const { return "HTTP/" + version_; }

  /// @brief Gets the header of HTTP Protocol.
  const HeaderPairs& GetHeader() const { return headers_; }
  HeaderPairs* GetMutableHeader() { return &headers_; }

  /// @brief Adds the header key-value pair.
  /// It appends to any existing values associated with key.
  template <typename K, typename V>
  void AddHeader(K&& key, V&& value) {
    headers_.Add(std::forward<K>(key), std::forward<V>(value));
  }
  /// @brief Sets the header associated with the key to the single element value.
  /// It replaces any existing values associated with key.
  template <typename K, typename V>
  void SetHeader(K&& key, V&& value) {
    headers_.Set(std::forward<K>(key), std::forward<V>(value));
  }

  /// @brief  Does same thing as SetHeader method, but only works when key does not exist in the header.
  template <typename K, typename V>
  void SetHeaderIfNotPresent(K&& key, V&& value) {
    headers_.SetIfNotPresent(std::forward<K>(key), std::forward<V>(value));
  }

  /// @brief Gets the first header value associated with the given key.
  /// If there are no associated with the key, it returns empty string.
  const std::string& GetHeader(std::string_view key) const { return headers_.Get(key); }

  /// @brief Returns all header values associated with the given key. If there are no associated values with the key,
  /// it returns emtpy list.
  std::vector<std::string_view> GetHeaderValues(std::string_view key) const { return headers_.Values(key); }

  /// @brief Returns all key-value pairs of the headers.
  std::vector<std::pair<std::string_view, std::string_view>> GetHeaderPairs() const { return headers_.Pairs(); }

  /// @brief Deletes the values associated with the given key in the headers.
  void DeleteHeader(const std::string& key) { headers_.Delete(key); }
  void DeleteHeader(std::string_view key, std::string_view value) { headers_.Delete(key, value); }
  void DeleteHeader(std::string_view key, std::string_view value,
                    const std::function<bool(std::string_view lhs, std::string_view rhs)>& value_equal_to) {
    headers_.Delete(key, value, value_equal_to);
  }

  /// @brief Calls function |f| sequentially for each key and value present in the headers .
  /// If |f| returns false, it stops the iteration.
  void RangeHeader(const std::function<bool(std::string_view key, std::string_view value)>& f) const {
    headers_.Range(f);
  }
  /// @brief Reports whether headers contain the key.
  bool HasHeader(std::string_view key) const { return headers_.Has(key); }

 private:
  // Sets major and minor number of HTTP version.
  void SetMajorMinorVersion(int major, int minor) {
    major_ = major;
    minor_ = minor;
  }

 protected:  // Request and Response object will derive from Header object.
  // Request method.
  MethodType method_type_{MethodType::GET};
  std::string method_name_{"GET"};

  // Request URI, e.g "/index.html?name=tom".
  std::string request_uri_;

  // HTTP Version, 1.1 by default.
  int major_{1};
  int minor_{1};
  mutable std::string version_{kVersion11};

  // Header fields.
  mutable HeaderPairs headers_;
};

}  // namespace http

}  // namespace trpc
