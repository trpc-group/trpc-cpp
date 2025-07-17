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

#include "trpc/util/string/string_util.h"

#include <limits>

namespace trpc::util {

void TrimInplace(std::string& s) {
  if (s.empty()) {
    return;
  }
  s.erase(0, s.find_first_not_of(' '));
  s.erase(s.find_last_not_of(' ') + 1);
}

std::string Trim(const std::string& s) {
  std::string res = s;
  TrimInplace(res);
  return res;
}

std::string_view TrimStringView(std::string_view s) {
  s.remove_prefix(std::min(s.find_first_not_of(' '), s.size()));
  s.remove_suffix(std::min(s.size() - s.find_last_not_of(' ') - 1, s.size()));
  return s;
}

std::string_view TrimPrefixStringView(std::string_view s, std::string_view prefix) {
  if (s.length() < prefix.length() || s.substr(0, prefix.length()) != prefix) {
    return s;
  }
  return s.substr(prefix.length());
}

std::string_view TrimSuffixStringView(std::string_view s, std::string_view suffix) {
  if (s.length() < suffix.length() || s.substr(s.length() - suffix.length()) != suffix) {
    return s;
  }
  return s.substr(0, s.length() - suffix.length());
}

std::unordered_map<std::string, std::string> SplitStringToMap(std::string const& in, char separator1, char separator2,
                                                              bool trim) {
  std::unordered_map<std::string, std::string> res;
  // split to substrings
  auto sub_str_vec = SplitString(in, separator1);
  // split each substring
  for (auto const& sub_str : sub_str_vec) {
    auto kv = SplitString(sub_str, separator2);
    if (kv.empty()) {
      continue;
    }
    // take the first one as the key
    auto key = trim ? Trim(kv[0]) : kv[0];
    if (key.empty()) {
      continue;
    }
    // value can be empty.
    if (kv.size() == 1) {
      res[key] = "";
      continue;
    }
    // Take the second one as the value
    res[key] = trim ? Trim(kv[1]) : kv[1];
  }

  return res;
}

}  // namespace trpc::util
