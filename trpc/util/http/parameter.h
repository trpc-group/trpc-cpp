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

#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "trpc/util/http/field_map.h"
#include "trpc/util/string_util.h"

namespace trpc::http {

/// @brief A map represents the key-value pairs in URI-Query string. The key is case-sensitive.
///
/// The query component is indicated by the first question mark ("?") character and terminated by a number
/// sign ("#") character or by the end of the URI.
///
/// RFC : https://www.rfc-editor.org/rfc/rfc3986#section-3.4
using QueryParameters = detail::FieldMap<detail::CaseSensitiveLess>;

// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/include/seastar/http/common.hh

/// @brief A map represents the key-value pairs in a URI-Path string (placeholder in path).
/// The key is case-sensitive.
class PathParameters {
 public:
  /// @brief Gets the value associated with the given key.
  /// If there are no associated with the key, it returns exception: "out_of_range".
  /// @note The key is case-sensitive.
  const std::string& Path(const std::string& key) const { return params_.at(key); }
  /// @brief It's same as the Path method.
  const std::string& At(const std::string& key) const { return Path(key); }

  /// @brief Overriding operator '[]'. Gets the value associated with the given key, the leading slash in the path
  /// was removed.
  /// If there are no associated with the key, it returns exception: "out_of_range".
  /// @note The key is case-sensitive.
  std::string operator[](const std::string& key) const {
    auto v = params_.at(key);
    if (!v.empty() && v[0] == '/') return v.substr(1);
    return v;
  }

  /// @brief Sets or Gets the key-value pair.
  template <typename K, typename V>
  void Set(K&& key, V&& value) {
    params_[std::forward<K>(key)] = std::forward<V>(value);
  }

  /// @brief Gets the value associated with the given key.
  /// If there are no associated with the key, it returns empty string.
  /// @note The key is case-sensitive.
  const std::string& Get(const std::string& key) const {
    static const std::string default_value;
    return Get(key, default_value);
  }

  /// @brief Gets the value associated with the given key.
  /// If there are no associated with the key, it returns default value passed by caller.
  /// @note The key is case-sensitive.
  const std::string& Get(const std::string& key, const std::string& default_value) const {
    auto found = params_.find(key);
    if (found == params_.end()) {
      return default_value;
    }
    return found->second;
  }

  /// @brief Returns all key-value pairs.
  std::vector<std::pair<std::string_view, std::string_view>> Pairs() const {
    std::vector<std::pair<std::string_view, std::string_view>> pairs;
    for (const auto& [key, value] : params_) {
      pairs.emplace_back(key, value);
    }
    return pairs;
  }

  /// @brief Deletes the values associated with the given key.
  void Delete(const std::string& key) { params_.erase(key); }

  /// @brief Calls f sequentially for each key and value present in the map.
  /// If f returns false, range stops the iteration.
  void Range(const std::function<bool(std::string_view key, std::string_view value)>& f) const {
    for (const auto& [key, value] : params_) {
      if (!f(key, value)) return;
    }
  }

  /// @brief Reports weather parameters contains the key.
  bool Has(const std::string& key) const { return params_.find(key) != params_.end(); }
  /// @brief Deprecated: use Has method instead.
  bool Exists(const std::string& key) const { return Has(key); }

  /// @brief Clears parameters.
  void Clear() { params_.clear(); }

  /// @brief Returns count of pairs map.
  std::size_t PairsCount() const { return params_.size(); }

 private:
  std::unordered_map<std::string, std::string> params_;
};
using Parameters = PathParameters;
// End of source codes that are from seastar.

/// @brief Parses key-value pairs from body who's content type is "application/x-www-form-urlencoded".
class BodyParameters {
 public:
  explicit BodyParameters(std::string const& body) { body_parameters_ = util::SplitStringToMap(body, '&', '='); }

 public:
  /// @brief Gets the parameter value associated with the given name.
  /// If there are no associated with the name, it returns empty string.
  std::string GetBodyParam(std::string const& name) const {
    if (auto itr = body_parameters_.find(name); itr != body_parameters_.end()) {
      return itr->second;
    }
    return "";
  }

  /// @brief Returns the size of parameters.
  uint32_t Size() const { return body_parameters_.size(); }

 private:
  // Key-value pairs, only supports content-type: application/x-www-form-urlencoded.
  std::unordered_map<std::string, std::string> body_parameters_;
};
using BodyParam = BodyParameters;

}  // namespace trpc::http
