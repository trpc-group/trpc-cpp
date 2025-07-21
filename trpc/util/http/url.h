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

#include <cinttypes>
#include <string>
#include <unordered_map>
#include <utility>

namespace trpc::http {

class Url;

/// @brief Parses a raw url into Url structure.
/// Try to parse a hostname and path without a scheme is valid.
///
/// @param url may be relative (a path, without a host), or absolute (starting with scheme).
/// @param parsed_url a pointer of Url structure.
/// @param err stores error message if error occurred.
/// @return  Returns true on success, false otherwise.
bool ParseUrl(std::string_view url, Url* parsed_url, std::string* err);

/// @brief A Url represents a parsed URL (technically, a URI reference).
/// A Uniform Resource Identifier (URI) is a unique sequence of characters that identifies a logical or
/// physical resource used by web technologies.
///
/// The class for URI scheme : http://en.wikipedia.org/wiki/URI_scheme
/// The following figure displays example URIs and their component parts.
///
///         userinfo       host      port
///         ┌──┴───┐ ┌──────┴──────┐ ┌┴┐
/// https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top
/// └─┬─┘   └─────────────┬────────────┘└───────┬───────┘ └────────────┬────────────┘ └┬┘
/// scheme          authority                  path                  query           fragment
class Url {
 public:
  const std::string& Scheme() const { return scheme_; }
  const std::string& Userinfo() const { return userinfo_; }
  const std::string& Host() const { return host_; }
  const std::string& Port() const { return port_; }
  const std::string& Path() const { return path_; }
  const std::string& RawPath() const { return raw_path_; }
  const std::string& Query() const { return query_; }
  const std::string& RawQuery() const { return raw_query_; }
  const std::string& Fragment() const { return fragment_; }
  std::string Username() const;
  std::string Password() const;
  int IntegerPort() const;

  std::string* MutableScheme() { return &scheme_; }
  std::string* MutableUserinfo() { return &userinfo_; }
  std::string* MutableHost() { return &host_; }
  std::string* MutablePort() { return &port_; }
  std::string* MutablePath() { return &path_; }
  std::string* MutableRawPath() { return &raw_path_; }
  std::string* MutableQuery() { return &query_; }
  std::string* MutableRawQuery() { return &raw_query_; }
  std::string* MutableFragment() { return &fragment_; }

  /// @brief Reports whether the URL is absolute. Absolute URL means that it has non-empty scheme.
  bool IsAbsolute() const { return !scheme_.empty(); }
  /// @brief Returns the "path?query#fragment" string that would be used in HTTP Request for Url object.
  /// e.g. Returns "/abc?a=b#ref" for "http://localhost/abc?a=b#ref".
  std::string RequestUri() const;

 private:
  std::string scheme_;
  std::string userinfo_;
  std::string host_;
  std::string port_;
  // Form after percent-encoding decoded. Relative path may omit leading slash.
  std::string path_;
  // Percent-encoded.
  std::string raw_path_;
  // Key-Value pairs joined with '&', without '?'
  std::string query_;
  // Percent-encoded.
  std::string raw_query_;
  // Fragment for references, without '#'.
  std::string fragment_;
};

/// @brief A URL parser, parses URL string.
class UrlParser {
 public:
  UrlParser() : ok_(false) {}

  /// @brief Allocates an Url object and parse a url, you should see OK() next.
  explicit UrlParser(std::string_view url) : ok_(false) { Parse(url); }

  /// @brief Parses a url, returns true if parsing is OK, false if not.
  bool Parse(std::string_view url) {
    url_ = Url();
    ok_ = ParseUrl(url, &url_, nullptr);
    return ok_;
  }

  /// @brief Reports whether url parses well.
  /// @return Returns true if parsing is ok, false otherwise.
  bool IsValid() const { return ok_; }

  std::string Scheme() const { return url_.Scheme(); }
  std::string Userinfo() const { return url_.Userinfo(); }
  std::string Username() const { return url_.Username(); }
  std::string Password() const { return url_.Password(); }
  std::string Hostname() const { return url_.Host(); }
  std::string Port() const { return url_.Port(); }
  std::string Path() const { return url_.Path(); }
  std::string Query() const { return url_.Query(); }
  std::string Fragment() const { return url_.Fragment(); }
  std::uint16_t IntegerPort() const { return url_.IntegerPort(); }

  /// @brief Returns the "path?query#fragment" string that would be used in HTTP Request for Url object.
  /// e.g. Returns "/abc?a=b#ref" for "http://localhost/abc?a=b#ref".
  std::string RequestUri() const { return url_.RequestUri(); }
  std::string Request() const { return url_.RequestUri(); }

  /// @brief Get parsed URL (move).
  Url GetUrl() { return std::move(url_); }

 private:
  bool ok_{false};
  Url url_{};
};

}  // namespace trpc::http
