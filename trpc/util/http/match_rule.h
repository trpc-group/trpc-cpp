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

#include <memory>
#include <string>
#include <vector>

#include "trpc/util/http/handler.h"
#include "trpc/util/http/matcher.h"
#include "trpc/util/http/parameter.h"

namespace trpc::http {

// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/include/seastar/http/matchrules.hh

/// @brief Checks if an URL string matches criteria which can have parameters. The 'http::Routes' object would call
/// the 'Get' method with an URL string as parameter, and if it matches successfully, the method will return a handler
/// when matching processes, the method will fill the parameters object.
class MatchRule {
 public:
  explicit MatchRule(HandlerBase* handler) : handler_(handler) {}
  explicit MatchRule(std::shared_ptr<HandlerBase> handler) : handler_(std::move(handler)) {}

  /// @brief Checks if url match the rule and return a handler.
  /// @param url is an url to compare against the rule.
  /// @param params then parameters object, matches parameters will fill the object.
  /// @return Returns a handler if there is a full match. nullptr otherwise.
  HandlerBase* Get(const std::string& url, Parameters& params) {
    size_t index = 0;
    for (auto& matcher : matchers_) {
      index = matcher->Match(url, index, params);
      if (index == std::string::npos) {
        return nullptr;
      }
    }
    return (index + 1 >= url.length()) ? handler_.get() : nullptr;
  }

  /// @brief Adds a matcher to the matching rules.
  /// @param match is the matcher to add.
  /// @return Returns a reference to the itself of 'MatchRule' object.
  MatchRule& AddMatcher(Matcher* match) { return AddMatcher(std::shared_ptr<Matcher>(match)); }
  MatchRule& AddMatcher(std::shared_ptr<Matcher> match) {
    matchers_.push_back(std::move(match));
    return *this;
  }

  /// @brief Adds a StringMatcher.
  /// @param str is the string to search for.
  /// @return Returns a reference to the itself of 'MatchRule' object.
  MatchRule& AddString(const std::string& str) {
    AddMatcher(std::make_shared<StringProxyMatcher>(str));
    return *this;
  }

  /// @brief Adds a parameter matcher to the rule.
  /// @param str is the parameter name.
  /// @param full_path when set to true, parameter will include all the reminding url until its end.
  /// @return Returns a reference to the itself of 'MatchRule' object.
  MatchRule& AddParam(const std::string& str, bool full_path = false) {
    AddMatcher(std::make_shared<ParamMatcher>(str, full_path));
    return *this;
  }

 private:
  std::shared_ptr<HandlerBase> handler_;
  std::vector<std::shared_ptr<Matcher>> matchers_;
};
// End of source codes that are from seastar.

}  // namespace trpc::http
