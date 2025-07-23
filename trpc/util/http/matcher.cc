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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. seastar
// Copyright (C) 2015 Cloudius Systems
// seastar is licensed under the Apache 2.0 License.
//
//

#include "trpc/util/http/matcher.h"

#include <assert.h>

namespace trpc::http {

// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/src/http/matcher.cc.

namespace {
/// @brief  Search for the end position for the url parameter.
/// @param url is the url to search.
/// @param start_pos is the start position to search in the url
/// @param entire_path when set to true, take all the reminding url, when set to false, search for the next flash.
/// @return the position in the url of the end of parameter
std::size_t FindEndParam(std::string_view url, std::size_t start_pos, bool entire_path) {
  std::size_t pos = (entire_path) ? url.length() : url.find("/", start_pos + 1);
  return pos == std::string::npos ? url.length() : pos;
}
}  // namespace

std::size_t ParamMatcher::Match(std::string_view url, std::size_t start_pos, PathParameters& param) {
  std::size_t last = FindEndParam(url, start_pos, entire_path_);
  if (last == start_pos) {
    // Empty parameter allows only for the case of entire path is set.
    if (entire_path_) {
      param.Set(std::string{name_}, "");
      return start_pos;
    }
    return std::string::npos;
  }
  param.Set(std::string{name_}, std::string{url.substr(start_pos, last - start_pos)});
  return last;
}

StringMatcher::StringMatcher(const std::string& cmp) : cmp_(cmp), len_(cmp.size()), regex_pattern_(std::nullopt) {
  std::smatch m;
  std::regex regex_pattern("^<regex(.*)>");
  if (!std::regex_match(cmp_, m, regex_pattern)) {
    return;
  }
  // The path filled in by the user is in regular expression format.
  regex_pattern_ = std::regex(m.str(1));
}

std::size_t StringMatcher::Match(std::string_view url, std::size_t start_pos, PathParameters& param) {
  // Prefix matching or exact matching.
  if (url.length() >= len_ + start_pos && (url.find(cmp_, start_pos) == start_pos) &&
      (url.length() == len_ + start_pos || url.at(len_ + start_pos) == '/')) {
    return len_ + start_pos;
  }
  if (regex_pattern_ && std::regex_search(url.cbegin(), url.cend(), regex_pattern_.value())) {
    return url.length();
  }
  return std::string::npos;
}
// End of source codes that are from seastar.

PlaceholderMatcher::PlaceholderMatcher(const std::string& input) : origin_(input) {
  std::smatch m;
  std::regex placeholder_pattern("<[\\w]+>");
  // Extracts placeholder name.
  std::string tmp = origin_;
  while (std::regex_search(tmp, m, placeholder_pattern)) {
    std::string placeholder_name = m.str();
    assert(placeholder_name.size() > 2);

    names_.push_back(placeholder_name.substr(1, placeholder_name.size() - 2));
    tmp = m.suffix();
  }
  // Replaces placeholder.
  tmp = origin_;
  cmp_ = regex_replace(tmp, placeholder_pattern, "([^/]+)");
  cmp_ = "^" + cmp_ + "$";
  // Generates regex pattern.
  pattern_ = std::regex(cmp_);
}

std::size_t PlaceholderMatcher::Match(std::string_view url, std::size_t start_pos, trpc::http::PathParameters& param) {
  std::match_results<std::string_view::const_iterator> m;
  if (!std::regex_match(url.cbegin(), url.cend(), m, pattern_)) {
    return std::string::npos;
  }
  assert(m.size() == names_.size() + 1);
  for (std::size_t i = 1; i < m.size(); ++i) {
    param.Set(names_[i - 1], m[i].str());
  }
  return url.size();
}

StringProxyMatcher::StringProxyMatcher(const std::string& cmp) {
  static std::regex placeholder_pattern("^<ph\\((.*)\\)>$");
  std::smatch m;
  if (std::regex_match(cmp, m, placeholder_pattern)) {
    assert(m.size() >= 2);
    matcher_ = std::make_unique<PlaceholderMatcher>(m.str(1));
    return;
  }
  matcher_ = std::make_unique<StringMatcher>(cmp);
}

std::size_t StringProxyMatcher::Match(std::string_view url, std::size_t start_pos, PathParameters& param) {
  return matcher_->Match(url, start_pos, param);
}

}  // namespace trpc::http
