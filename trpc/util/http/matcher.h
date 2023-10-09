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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. seastar
// Copyright (C) 2015 Cloudius Systems
// seastar is licensed under the Apache 2.0 License.
//
//

#pragma once

#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include "trpc/util/http/parameter.h"

namespace trpc::http {

// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/include/seastar/http/matcher.hh.

/// @brief An interface represents the ability to find a matched route rule for specified request URI path.
class Matcher {
 public:
  virtual ~Matcher() = default;

  /// @brief Reports whether URL matches a route rule.
  ///
  /// @param url is the request URI's path.
  /// @param start_pos is the start position of |uri_path|.
  /// @param param is matched path parameter.
  /// @return Returns matched end position of |uri_path| on success. `std::string::npos` on failure.
  virtual std::size_t Match(std::string_view url, std::size_t start_pos, PathParameters& param) = 0;
};

/// @brief Matches a specified parameter name in URI path, and fill the parameter with matched value. A non-empty URL
/// can always be successfully matched and filled with parameters.
class ParamMatcher : public Matcher {
 public:
  explicit ParamMatcher(const std::string& name, bool entire_path = false) : name_(name), entire_path_(entire_path) {}
  ~ParamMatcher() override = default;

  std::size_t Match(std::string_view url, std::size_t start_pos, PathParameters& param) override;

 private:
  // Parameter name.
  std::string name_;
  // Indicates to match the entire path.
  bool entire_path_;
};

/// @brief String matcher that supports regular expression matching
class StringMatcher : public Matcher {
 public:
  explicit StringMatcher(const std::string& cmp);
  ~StringMatcher() override = default;

  std::size_t Match(std::string_view url, std::size_t start_pos, PathParameters& param) override;

 private:
  // Matching rule.
  std::string cmp_;
  // Length of matching rule.
  std::size_t len_;
  std::optional<std::regex> regex_pattern_;
};
// End of source codes that are from seastar.

/// @brief Path placeholder matcher.
class PlaceholderMatcher : public trpc::http::Matcher {
 public:
  explicit PlaceholderMatcher(const std::string& input);
  ~PlaceholderMatcher() override = default;

  std::size_t Match(std::string_view url, std::size_t start_pos, PathParameters& param) override;

 private:
  // Original content of matching rule.
  std::string origin_;
  // Matching rule.
  std::string cmp_;
  // Regular expression pattern of matching rule.
  std::regex pattern_;
  // Placeholder names.
  std::vector<std::string> names_;
};

/// @brief Matcher proxy, used to adapt string matching and placeholder matching.
class StringProxyMatcher : public Matcher {
 public:
  explicit StringProxyMatcher(const std::string& cmp);
  ~StringProxyMatcher() override = default;

  std::size_t Match(std::string_view url, std::size_t start_pos, PathParameters& param) override;

 private:
  std::unique_ptr<Matcher> matcher_ = nullptr;
};

}  // namespace trpc::http
