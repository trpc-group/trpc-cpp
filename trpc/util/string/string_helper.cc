//
//
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
// Flare is licensed under the BSD 3-Clause License.
// The source codes in this file based on
// https://github.com/Tencent/flare/blob/master/flare/base/string.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the BSD 3-Clause License.
//
//

#include "trpc/util/string/string_helper.h"

#include <array>
#include <cstdlib>
#include <climits>

#include "trpc/util/check.h"
#include "trpc/util/likely.h"

namespace trpc {

namespace {

constexpr std::array<char, 256> kLowerChars = []() {
  std::array<char, 256> cs{};
  for (std::size_t index = 0; index != 256; ++index) {
    if (index >= 'A' && index <= 'Z') {
      cs[index] = index - 'A' + 'a';
    } else {
      cs[index] = index;
    }
  }
  return cs;
}();

constexpr std::array<char, 256> kUpperChars = []() {
  std::array<char, 256> cs{};
  for (std::size_t index = 0; index != 256; ++index) {
    if (index >= 'a' && index <= 'z') {
      cs[index] = index - 'a' + 'A';
    } else {
      cs[index] = index;
    }
  }
  return cs;
}();

template <class T, class F, class... Args>
std::optional<T> TryParseImpl(F fptr, const char* s, std::initializer_list<T> invs, Args... args) {
  if (TRPC_UNLIKELY(s[0] == 0)) {
    return std::nullopt;
  }
  char* end;
  auto result = fptr(s, &end, args...);
  if (end != s + strlen(s)) {  // Return value `0` is also handled by this test.
    return std::nullopt;
  }
  for (auto&& e : invs) {
    if (result == e && errno == ERANGE) {
      return std::nullopt;
    }
  }
  return result;
}

template <class T, class U>
std::optional<T> TryNarrowCast(U&& opt) {
  if (!opt || *opt > std::numeric_limits<T>::max() || *opt < std::numeric_limits<T>::min()) {
    return std::nullopt;
  }
  return opt;
}

template <class S>
void JoinImpl(const std::vector<S>& parts, const std::string_view& delim, std::string* result) {
  auto size = 0;
  for (auto&& e : parts) {
    size += e.size() + delim.size();
  }
  result->clear();
  if (!size) {
    return;
  }
  size -= delim.size();
  result->reserve(size);
  for (auto iter = parts.begin(); iter != parts.end(); ++iter) {
    if (iter != parts.begin()) {
      result->append(delim.begin(), delim.end());
    }
    result->append(iter->begin(), iter->end());
  }
  return;
}

}  // namespace

bool StartsWith(const std::string_view& s, const std::string_view& prefix) {
  return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

bool EndsWith(const std::string_view& s, const std::string_view& suffix) {
  return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

void Replace(const std::string_view& from, const std::string_view& to, std::string* str, std::size_t count) {
  TRPC_CHECK(!from.empty(), "`from` may not be empty.");
  auto p = str->find(from);
  while (p != std::string::npos && count--) {
    str->replace(p, from.size(), to);
    p = str->find(from, p + to.size());
  }
}

std::string Replace(const std::string_view& str, const std::string_view& from, const std::string_view& to,
                    std::size_t count) {
  std::string cp(str);
  Replace(from, to, &cp, count);
  return cp;
}

std::string_view Trim(const std::string_view& str) {
  std::size_t s = 0, e = str.size();
  if (str.front() == ' ') {
    s = str.find_first_not_of(' ');
  }
  if (s == std::string_view::npos) {
    return {};
  }
  if (str.back() == ' ') {
    e = str.find_last_not_of(' ');
  }
  return str.substr(s, e - s + 1);
}

std::vector<std::string_view> Split(const std::string_view& s, char delim, bool keep_empty) {
  return Split(s, std::string_view(&delim, 1), keep_empty);
}

std::vector<std::string_view> Split(const std::string_view& s, const std::string_view& delim, bool keep_empty) {
  std::vector<std::string_view> splited;
  if (s.empty()) {
    return splited;
  }
  auto current = s;
  TRPC_CHECK(!delim.empty());
  while (true) {
    auto pos = current.find(delim);
    if (pos != 0 || keep_empty) {
      splited.push_back(current.substr(0, pos));
    }  // Empty part otherwise.
    if (pos == std::string_view::npos) {
      break;
    }
    current = current.substr(pos + delim.size());
    if (current.empty()) {
      if (keep_empty) {
        splited.push_back("");
      }
      break;
    }
  }
  return splited;
}

std::string Join(const std::vector<std::string_view>& parts, const std::string_view& delim) {
  std::string result;
  JoinImpl(parts, delim, &result);
  return result;
}

std::string Join(const std::vector<std::string>& parts, const std::string_view& delim) {
  std::string result;
  JoinImpl(parts, delim, &result);
  return result;
}

char ToUpper(char c) { return kUpperChars[c]; }
char ToLower(char c) { return kLowerChars[c]; }

void ToUpper(std::string* s) {
  for (auto&& c : *s) {
    c = ToUpper(c);
  }
}

void ToLower(std::string* s) {
  for (auto&& c : *s) {
    c = ToLower(c);
  }
}

std::string ToUpper(const std::string_view& s) {
  std::string result;
  result.reserve(s.size());
  for (auto&& e : s) {
    result.push_back(ToUpper(e));
  }
  return result;
}

std::string ToLower(const std::string_view& s) {
  std::string result;
  result.reserve(s.size());
  for (auto&& e : s) {
    result.push_back(ToLower(e));
  }
  return result;
}

bool IEquals(const std::string_view& first, const std::string_view& second) {
  if (first.size() != second.size()) {
    return false;
  }
  for (std::size_t index = 0; index != first.size(); ++index) {
    if (ToLower(first[index]) != ToLower(second[index])) {
      return false;
    }
  }
  return true;
}

std::optional<bool> TryParseTraits<bool>::TryParse(const std::string_view& s, bool recognizes_alphabet_symbol,
                                                   bool ignore_case) {
  if (auto num_opt = trpc::TryParse<int>(s); num_opt) {
    if (*num_opt == 0) {
      return false;
    } else if (*num_opt == 1) {
      return true;
    }
    return std::nullopt;
  }
  if (IEquals(s, "y") || IEquals(s, "yes") || IEquals(s, "true")) {
    return true;
  } else if (IEquals(s, "n") || IEquals(s, "no") || IEquals(s, "false")) {
    return false;
  }
  return std::nullopt;
}

template <class T>
std::optional<T> TryParseTraits<T, std::enable_if_t<std::is_integral_v<T>>>::TryParse(const std::string_view& s,
                                                                                      int base) {
  auto str = std::string(s);  // Slow.

  // Here we always use the largest type to hold the result, and check if the
  // result is actually larger than what `T` can hold.
  //
  // It's not very efficient, though.
  if constexpr (std::is_signed_v<T>) {
    auto opt = TryParseImpl<std::int64_t>(&strtoll, str.c_str(), {LLONG_MIN, LLONG_MAX}, base);
    return TryNarrowCast<T>(opt);
  } else {
    auto opt = TryParseImpl<std::uint64_t>(&strtoull, str.c_str(), {ULLONG_MAX}, base);
    return TryNarrowCast<T>(opt);
  }
}

template <class T>
std::optional<T> TryParseTraits<T, std::enable_if_t<std::is_floating_point_v<T>>>::TryParse(const std::string_view& s) {
  auto str = std::string(s);  // Slow.
  // We cannot use the same trick as `TryParse<integral>` here, as casting
  // between floating point is lossy.
  if (std::is_same_v<T, float>) {
    return TryParseImpl<float>(&strtof, str.c_str(), {-HUGE_VALF, HUGE_VALF});
  } else if (std::is_same_v<T, double>) {
    return TryParseImpl<double>(&strtod, str.c_str(), {-HUGE_VAL, HUGE_VAL});
  } else if (std::is_same_v<T, long double>) {
    return TryParseImpl<long double>(&strtold, str.c_str(), {-HUGE_VALL, HUGE_VALL});
  }
  TRPC_CHECK(0);
  return {};
}

#define INSTANTIATE_TRY_PARSE_TRAITS(type) template struct TryParseTraits<type>;

INSTANTIATE_TRY_PARSE_TRAITS(int8_t);
INSTANTIATE_TRY_PARSE_TRAITS(int16_t);
INSTANTIATE_TRY_PARSE_TRAITS(int32_t);
INSTANTIATE_TRY_PARSE_TRAITS(int64_t);
INSTANTIATE_TRY_PARSE_TRAITS(uint8_t);
INSTANTIATE_TRY_PARSE_TRAITS(uint16_t);
INSTANTIATE_TRY_PARSE_TRAITS(uint32_t);
INSTANTIATE_TRY_PARSE_TRAITS(uint64_t);
INSTANTIATE_TRY_PARSE_TRAITS(float);
INSTANTIATE_TRY_PARSE_TRAITS(double);
INSTANTIATE_TRY_PARSE_TRAITS(long double);

#undef INSTANTIATE_TRY_PARSE_INTEGRAL
#undef INSTANTIATE_TRY_PARSE_FLOATING_POINT

}  // namespace trpc
