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

#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace trpc::http {

/// @brief Namespace of details for http sub-module.
/// @private For internal use purpose only.
namespace detail {
/// @brief Compares two strings, reports whether |lhs| less than |rhs|.
/// @private For internal use purpose only.
struct CaseSensitiveLess : public std::less<> {
  bool operator()(std::string_view lhs, std::string_view rhs) const { return lhs < rhs; }
};

/// @brief Compares two strings, reports whether |lhs| less than |rhs|. (Case ignored)
/// @private For internal use purpose only.
struct CaseInsensitiveLess : public std::less<> {
  bool operator()(std::string_view lhs, std::string_view rhs) const {
    // HTTP/2 uses special pseudo-header fields beginning with ':' character (ASCII 0x3a).
    // All pseudo-header fields MUST appear in the header block before regular header fields.
    if (lhs.size() != rhs.size() && lhs.front() != ':' && rhs.front() != ':') {
      return lhs.size() < rhs.size();
    }
    return strncasecmp(lhs.data(), rhs.data(), lhs.size()) < 0;
  }
};

/// @brief Compares two strings, reports whether |lhs| equals |rhs|. (Case ignored)
/// @private For internal use purpose only.
struct CaseInsensitiveEqualTo : public std::equal_to<> {
  bool operator()(std::string_view lhs, std::string_view rhs) const {
    return lhs.size() == rhs.size() && strncasecmp(lhs.data(), rhs.data(), lhs.size()) == 0;
  }
};

/// @brief Comparison function which returns true if the first argument and the second argument are equal.
/// @private For internal use purpose only.
using EqualTo = std::function<bool(std::string_view lhs, std::string_view rhs)>;

/// @brief A map represents the key-value pairs, a key may map to many values, key is compared by "Compare" parameter.
/// @private For internal use purpose only.
template <typename Compare>
class FieldMap {
 public:
  /// @brief Adds the key-value pair to the map.
  /// It appends to any existing values associated with key.
  template <typename K, typename V>
  void Add(K&& key, V&& value) {
    pairs_.emplace(std::forward<K>(key), std::forward<V>(value));
  }

  /// @brief Sets the map entries associated with the key to the single element value.
  /// It replaces any existing values associated with key.
  template <typename K, typename V>
  void Set(K&& key, V&& value) {
    Delete(key);
    Add(std::forward<K>(key), std::forward<V>(value));
  }

  /// @brief  Does same thing as Set method, but only works when key does not exist in the map.
  template <typename K, typename V>
  void SetIfNotPresent(K&& key, V&& value) {
    if (auto it = pairs_.lower_bound(key); it == pairs_.end() || !CaseInsensitiveEqualTo()(it->first, key)) {
      pairs_.emplace_hint(it, std::forward<K>(key), std::forward<V>(value));
    }
  }

  /// @brief Gets the first value associated with the given key.
  /// If there are no associated with the key, it returns empty string.
  const std::string& Get(std::string_view key) const {
    auto found = pairs_.find(key);
    if (found == pairs_.end()) {
      static const std::string default_value;
      return default_value;
    }
    return found->second;
  }

  /// @brief Deletes the values associated with the given key.
  void Delete(const std::string& key) { pairs_.erase(key); }

  /// @brief Deletes the value associated with the given key when values are equal.
  /// The value is case-sensitive.
  void Delete(std::string_view key, std::string_view value) {
    constexpr static std::equal_to<> value_equal_to;
    Delete(key, value, value_equal_to);
  }

  /// @brief Deletes the value associated with the given key when values are equal.
  /// Values are compared by value_equal_to function.
  void Delete(std::string_view key, std::string_view value, const EqualTo& value_equal_to) {
    auto [begin, end] = pairs_.equal_range(key);
    for (auto iter = begin; iter != end;) {
      auto pos = iter++;
      if (value_equal_to(pos->second, value)) {
        pairs_.erase(pos);
      }
    }
  }

  /// @brief Returns all values associated with the given key. If there are no associated values with the key,
  /// it returns emtpy list.
  std::vector<std::string_view> Values(std::string_view key) const {
    std::vector<std::string_view> values;
    auto [iter, end] = ValueIterator(key);
    for (; iter != end; iter++) {
      values.emplace_back(iter->second);
    }
    return values;
  }

  /// @brief Returns all key-value pairs.
  std::vector<std::pair<std::string_view, std::string_view>> Pairs() const {
    std::vector<std::pair<std::string_view, std::string_view>> pairs;
    for (const auto& [key, value] : pairs_) {
      pairs.emplace_back(key, value);
    }
    return pairs;
  }

  /// @brief Calls f sequentially for each key and value present in the map.
  /// If f returns false, range stops the iteration.
  void Range(const std::function<bool(std::string_view key, std::string_view value)>& f) const {
    for (auto iter = pairs_.cbegin(); iter != pairs_.cend(); iter++) {
      if (!f(iter->first, iter->second)) {
        return;
      }
    }
  }

  /// @brief Reports whether map contains the key.
  bool Has(std::string_view key) const { return pairs_.find(key) != pairs_.end(); }

  /// @brief Returns a string for all key-value pairs.
  /// Format: *("Key: Value\r\n")
  std::string ToString() const {
    std::string content;
    content.reserve(ByteSizeLong());
    for (const auto& [key, value] : pairs_) {
      content.append(key);
      content.append(kSeparator);
      content.append(value);
      content.append(kDelimiter);
    }
    return content;
  }

  /// @brief Writes key-value pairs to string.
  template <typename T>
  void WriteToStringStream(T& stream, std::string_view delimiter = kDelimiter) const {
    for (const auto& [key, value] : pairs_) {
      stream << key << kSeparator << value << delimiter;
    }
  }

  /// @brief Returns byte size long for all key-value pairs in string format.
  std::size_t ByteSizeLong() const {
    std::size_t byte_size{0};
    for (const auto& [key, value] : pairs_) {
      // Appends: Separator + Delimiter
      byte_size += key.size() + value.size() + kSeparator.size() + kDelimiter.size();
    }
    return byte_size;
  }

  /// @brief Returns count of pairs in flat format.
  std::size_t FlatPairsCount() const { return pairs_.size(); }

  /// @brief Clears the map.
  void Clear() { pairs_.clear(); }

  /// @brief Returns key-value pairs iterator of specific key.
  using ConstPairtIterator = typename std::multimap<std::string, std::string, Compare>::const_iterator;
  std::pair<ConstPairtIterator, ConstPairtIterator> ValueIterator(std::string_view key) const {
    return pairs_.equal_range(key);
  }

  /// @brief Reports whether key-value pairs is empty.
  bool Empty() const { return pairs_.empty(); }

  /// @brief Returns size of key-values pairs.
  std::size_t Size() const { return pairs_.size(); }

 private:
  static constexpr std::string_view kSeparator{": "};
  static constexpr std::string_view kDelimiter{"\r\n"};

  std::multimap<std::string, std::string, Compare> pairs_;
};

}  // namespace detail

}  // namespace trpc::http
