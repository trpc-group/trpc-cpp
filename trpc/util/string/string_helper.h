//
//
// Copyright (C) 2019 Tencent. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/string.h.
// This source file may have been modified by Tencent, and licensed under the BSD 3-Clause License.
//
//

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "fmt/format.h"

namespace trpc {

template <class T, class = void>
struct TryParseTraits;

/// @brief Try parse `s` as `T`.
/// @note To "specialize" `trpc::TryParse` for your own type, either implement
///       `TryParse` in a namespace that can be found via ADL, with signature like ...
template <class T, class... Args>
inline std::optional<T> TryParse(const std::string_view& s, const Args&... args) {
  return TryParseTraits<T>::TryParse(s, args...);
}

/// @sa: `std::format`
template <class... Args>
std::string Format(const std::string_view& fmt, const Args&... args) {
  #if FMT_VERSION >= 80000
  return fmt::format(fmt::runtime(fmt), args...);
  #else
  return fmt::format(fmt, args...);
  #endif
}

/// @brief `std::string(_view)::starts_with/ends_with` is not available until C++20, so
///         we roll our own here.
bool StartsWith(const std::string_view& s, const std::string_view& prefix);
bool EndsWith(const std::string_view& s, const std::string_view& suffix);

/// @brief Replace occurrance of `from` in `str` to `to` for at most `count` times.
void Replace(const std::string_view& from, const std::string_view& to, std::string* str,
             std::size_t count = std::numeric_limits<std::size_t>::max());
std::string Replace(const std::string_view& str, const std::string_view& from, const std::string_view& to,
                    std::size_t count = std::numeric_limits<std::size_t>::max());

/// @brief Trim whitespace at both end of the string.
std::string_view Trim(const std::string_view& str);

/// @brief Split string by `delim`.
std::vector<std::string_view> Split(const std::string_view& s, char delim, bool keep_empty = false);
std::vector<std::string_view> Split(const std::string_view& s, const std::string_view& delim, bool keep_empty = false);

/// @brief Join strings in `parts`, delimited by `delim`.
std::string Join(const std::vector<std::string_view>& parts, const std::string_view& delim);
std::string Join(const std::vector<std::string>& parts, const std::string_view& delim);

/// @brief To uppercase / lowercase.
char ToUpper(char c);
char ToLower(char c);
void ToUpper(std::string* s);
void ToLower(std::string* s);
std::string ToUpper(const std::string_view& s);
std::string ToLower(const std::string_view& s);

/// @brief Case insensitive-comparison.
/// @note  Use `ICompare` instead when C++20 is available
///       `ICompare` function is reserved and no implement for now,
///        so we can implement it and return `std::strong_ordering` once C++20 is available.
bool IEquals(const std::string_view& first, const std::string_view& second);



/// @brief Implementation goes below.
template <class T, class>
struct TryParseTraits {
  // The default implementation delegates calls to `TryParse` found by ADL.
  template <class... Args>
  static std::optional<T> TryParse(const std::string_view& s, const Args&... args) {
    return TryParse(std::common_type<T>(), s, args...);
  }
};

template <>
struct TryParseTraits<bool> {
  // For numerical values, only 0 and 1 are recognized, all other numeric values
  // lead to parse failure (i.e., `std::nullopt` is returned).
  //
  // If `recognizes_alphabet_symbol` is set, following symbols are recognized:
  //
  // - "true" / "false"
  // - "y" / "n"
  // - "yes" / "no"
  //
  // For all other symbols, we treat them as neither `true` nor `false`.
  // `std::nullopt` is returned in those cases.
  static std::optional<bool> TryParse(const std::string_view& s, bool recognizes_alphabet_symbol = true,
                                      bool ignore_case = true);
};

template <class T>
struct TryParseTraits<T, std::enable_if_t<std::is_integral_v<T>>> {
  static std::optional<T> TryParse(const std::string_view& s, int base = 10);
};

template <class T>
struct TryParseTraits<T, std::enable_if_t<std::is_floating_point_v<T>>> {
  static std::optional<T> TryParse(const std::string_view& s);
};

}  // namespace trpc
