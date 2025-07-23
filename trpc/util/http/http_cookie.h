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
#include <unordered_map>

namespace trpc::http {

/// @brief This class represents an HTTP Cookie.
/// Reference to RFC 2109.
/// @sa <https://datatracker.ietf.org/doc/html/rfc2109>
class HttpCookie {
 public:
  // The RFC version number for HTTP state management convention, with a current value of 0 or 1.
  // Default value is 0.
  // 0: Uses the Netscape cookie format.
  // 1: Uses the RFC2109 cookie format.
  static constexpr int kVersionNetscape{0};
  static constexpr int kVersionRfc2109{1};

  static constexpr int kSameSiteNotSpecified{0};
  static constexpr int kSameSiteNone{1};
  static constexpr int kSameSiteLax{2};
  static constexpr int kSameSiteStrict{3};

  /// @brief Percent-encoding.
  static std::string Escape(const std::string& str);
  /// @brief Percent-decoding.
  static std::string Unescape(const std::string& str);

 public:
  HttpCookie();
  HttpCookie(const std::string& name, const std::string& value);
  explicit HttpCookie(const std::unordered_map<std::string, std::string>& name_value_pairs);

  /// @brief Sets the version of the cookie.
  /// @note version must be 0 or 1. 0：Netscape cookie, 1：RFC2019 cookie.
  void SetVersion(int version) { version_ = version; }

  /// @brief Gets the version of the cookie.
  int GetVersion() const { return version_; }

  /// @brief Sets the name of the cookie.
  void SetName(const std::string& name) { name_ = name; }

  /// @brief Gets the name of the cookie.
  const std::string& GetName() const { return name_; }

  /// @brief Sets the value of the cookie.
  /// @note The value should be escaped if it contains whitespace or non-alphanumeric characters.
  void SetValue(const std::string& value) { value_ = value; }

  /// @brief Gets the value of the cookie.
  const std::string& GetValue() const { return value_; }

  /// @brief Sets the comments for the cookie.
  void SetComment(const std::string& comment) { comment_ = comment; }

  /// @brief Gets the comments for the cookie.
  const std::string& GetComment() const { return comment_; }

  /// @brief Sets the domain for the cookie.
  void SetDomain(const std::string& domain) { domain_ = domain; }

  /// @brief Gets the domain for the cookie.
  const std::string& GetDomain() const { return domain_; }

  /// @brief Sets the path for the cookie.
  void SetPath(const std::string& path) { path_ = path; }

  /// @brief Gets the path for the cookie.
  const std::string& GetPath() const { return path_; }

  /// @brief Sets the value of secure flag for the cookie.
  void SetSecure(bool secure) { secure_ = secure; }

  /// @brief Gets the value of secure flag for the cookie.
  bool GetSecure() const { return secure_; }

  /// @brief Sets the max age in seconds for the cookie.
  void SetMaxAge(int max_age) { max_age_ = max_age; }

  /// @brief Gets the max age in seconds for the cookie.
  int GetMaxAge() const { return max_age_; }

  /// @brief Sets the value of http-only flag for the cookie.
  void SetHttpOnly(bool http_only = true) { http_only_ = http_only; }

  /// @brief Gets the value of http-only flag for the cookie.
  bool GetHttpOnly() const { return http_only_; }

  /// @brief Sets the cookies "SameSite" attribute.
  void SetSameSite(int same_site) { same_site_ = same_site; }

  /// @brief Gets the cookies "SameSite" attribute.
  int GetSameSite() const { return same_site_; }

  /// @brief Returns a string representation of the cookie, it's suitable for use in header: "Set-Cookie: ${cookie}".
  std::string ToString() const;

 private:
  // @brief Netscape cookie format.
  void ToStringVersionZero(std::string* result) const;
  // @brief RFC2109 cookie format.
  void ToStringVersionOne(std::string* result) const;
  void SetCookieFieldNameValue(const std::string& name, const std::string& value);

 private:
  int version_{kVersionNetscape};
  std::string name_;
  std::string value_;
  std::string comment_;
  std::string domain_;
  std::string path_;
  bool secure_{false};
  int max_age_{-1};
  bool http_only_{false};
  int same_site_{kSameSiteNotSpecified};
};

}  // namespace trpc::http
