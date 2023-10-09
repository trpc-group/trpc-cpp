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

#include <cctype>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fmt/core.h"
#include "fmt/format.h"
#include "fmt/printf.h"
#include "fmt/std.h"

namespace trpc::util {

/// @brief Remove leading and trailing whitespace characters from a string in place.
/// @param s The string to be processed
void TrimInplace(std::string& s);

/// @brief Remove leading and trailing whitespace characters from a string.
/// @param s The string to be processed, which cannot be modified
/// @return std::string The resulting string after removing whitespace characters
std::string Trim(const std::string& s);

/// @brief Remove leading and trailing whitespace characters from a string view.
/// @param s The string view to be processed
/// @return std::string_view The resulting string view after removing whitespace characters
std::string_view TrimStringView(std::string_view s);

/// @brief Remove a prefix from a string and return the resulting string view.
/// @param s The string to be processed
/// @param prefix The prefix to be removed
/// @return std::string_view The resulting string view after removing the prefix
/// @note If s does not start with prefix, return the original string view of s
std::string_view TrimPrefixStringView(std::string_view s, std::string_view prefix);

/// @brief Remove a suffix from a string and return the resulting string view.
/// @param s The string to be processed
/// @param suffix The suffix to be removed
/// @return std::string_view The resulting string view after removing the suffix
/// @note If s does not end with suffix, return the original string view of s
std::string_view TrimSuffixStringView(std::string_view s, std::string_view suffix);

/// @brief Split a string into substrings based on a specified separator character.
/// @param in The string to be processed
/// @param separator The separator character
/// @return std::vector The container storing the resulting substrings
template <typename T = std::string>
std::vector<T> SplitString(std::string_view in, char separator) {
  std::vector<T> output;
  auto begin = in.begin(), end = in.end();
  auto first = begin;

  while (first != end) {
    auto second = std::find(first, end, separator);
    if (first != second) output.emplace_back(in.substr(std::distance(begin, first), std::distance(first, second)));
    if (second == end) break;
    first = ++second;
  }

  return output;
}

/// @brief This is an inline function that splits a std::string_view into substrings based on a separator character.
/// @return std::vector of std::string_view objects.
inline std::vector<std::string_view> SplitStringView(std::string_view in, char separator) {
  return SplitString<std::string_view>(in, separator);
}

/// @brief Split a string into substrings based on two specified separator characters,
///        and then split each substring into a key-value pair.
///        for example:
///        in: "a=1&b=2"
///        separator1: '&'
///        separator2: '='
///        return {{a, 1}, {b, 2}}
/// @note
///  1. Empty substrings are discarded after splitting.
///  for example:
///      in: "=a=1&"
///      separator1: '&'
///      separator2: '='
///      After the first step of splitting, "in" is split into one substring "=a=1",
///      and the empty substring after the last '&' is discarded.
///      After the second step of splitting, "in" is split into "a" and "1",
///      and the empty substring before the first '=' is discarded.
///  2. After splitting with separator2, the first value is used as the key of the map,
///     and the second value is used as the value of the map. This means that:
///      a. If there is no value after splitting, it is filtered out.
///      b. If the key is empty after trimming, the data is discarded.
///      c. If there is only a key after splitting, the value is set to an empty string.
///      d. If there are multiple values after splitting with separator2,
///         only the first two are used, and the rest are discarded.
///  3. If there are multiple duplicate keys, the later results will overwrite the earlier ones.
/// @param in The original data
/// @param separator1 The separator character used to split "in" into substrings
/// @param separator2 The separator character used to split each substring into a key-value pair
/// @param trim Whether to trim the resulting data (remove leading and trailing whitespace)
/// @return std::unordered_map<std::string, std::string> e.g. a=1&b=2 -> {{a, 1}, {b, 2}}
std::unordered_map<std::string, std::string> SplitStringToMap(std::string const& in, char separator1, char separator2,
                                                  bool trim = true);

/// @brief Convert between string and numeric types.
/// @param t The input data
template <class OutType, class InType>
OutType Convert(const InType& t) {
  std::stringstream stream;

  stream << t;
  OutType result;
  stream >> result;

  return result;
}

/// @brief Join elements in a sequence into a new string with a specified separator character.
/// @tparam T The template data type, which must be convertible to a string
/// @param in The input sequence of elements to be converted
/// @param sep The separator character
template <class T>
std::string Join(const std::vector<T>& in, const std::string& sep) {
  std::stringstream stream;

  bool first = true;
  for (const auto& s : in) {
    if (first)
      first = false;
    else
      stream << sep;

    stream << s;
  }

  return stream.str();
}

/// @brief Add a prefix and suffix to a string.
/// @tparam T The input data type, which must be convertible to a string
/// @param in The input data
/// @param prefix The prefix
/// @param suffix The suffix
/// @return The combined string
template <class T>
std::string ModifyString(const T& in, const std::string& prefix, const std::string& suffix) {
  std::stringstream stream;
  stream << prefix << in << suffix;
  return stream.str();
}

/// @brief Format a string
/// @param fmt The format pattern
/// @param args The data to be converted
/// @return The formatted string
template <class S, class... T>
std::string FormatString(const S& fmt, T&&... args) {
  return fmt::format(fmt::runtime(fmt), std::forward<T>(args)...);
}

/// @brief Format arguments and return as a string
/// @param fmt The format pattern
/// @param args The data to be converted
/// @return The formatted string
template <class S, class... T>
std::string PrintString(const S& fmt, const T&... args) {
  return fmt::sprintf(fmt, args...);
}

}  // namespace trpc::util
