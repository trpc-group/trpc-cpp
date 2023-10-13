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
// 1. nghttp2
// Copyright (c) 2013 Tatsuhiro Tsujikawa
// nghttp2 is licensed under the MIT License.
//
//

#pragma once

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>

namespace trpc::http {

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/// @brief Decodes a url string.
bool UrlDecode(const std::string_view& in, std::string& out);

/// @brief Converts a hex encoded 2-bytes substring to char.
char HexstrToChar(const std::string_view& in, size_t from);

/// @brief Converts a hex to char.
char HexToByte(char c);

/// @brief Normalize the url to remove the last slash if exists.
/// @param url is the full url path.
/// @return returns the url form the request without the last slash.
std::string NormalizeUrl(std::string_view url);

// The following source codes are from nghttp2.
// Copied and modified from
// https://github.com/nghttp2/nghttp2/blob/v1.21.0/src/util.h

// @brief Checks if a character is an RFC3986 unreserved character.
bool InRfc3986UnreservedChars(const char c);

// @brief Checks if a character is an RFC3986 sub-delimiter.
bool InRfc3986SubDelimiters(const char c);

// @brief Returns true if |c| is in token (HTTP-p1, Section 3.2.6)
bool InToken(char c);
bool InAttrChar(char c);

// @brief Converts hexadecimal characters to integer values. Returns 256 for invalid characters.
uint32_t HexToUint(char c);

// @brief Parses a Non-NULL-terminated string and attempt to convert it to an unsigned integer.
// Success: Returns the parsed integer value. Failure: Returns -1.
int64_t ParseUint(const uint8_t* s, size_t len);

// @brief Encodes unreserved characters in the URI using percent encoding.
std::string PercentEncode(const unsigned char* target, size_t len);
std::string PercentEncode(const std::string& target);

// @brief Encodes unreserved characters in the URI-Path using percent encoding.
std::string PercentEncodePath(const std::string& s);

// @brief Decodes percent encoding characters URL.
std::string PercentDecode(const std::string& s);
template <typename InputIt>
std::string PercentDecode(InputIt first, InputIt last) {
  std::string result;
  result.resize(last - first);
  auto p = std::begin(result);
  for (; first != last; ++first) {
    if (*first != '%') {
      *p++ = *first;
      continue;
    }

    if (first + 1 != last && first + 2 != last && std::isxdigit(static_cast<uint8_t>(*(first + 1))) &&
        std::isxdigit(static_cast<uint8_t>(*(first + 2)))) {
      *p++ = (HexToUint(*(first + 1)) << 4) + HexToUint(*(first + 2));
      first += 2;
      continue;
    }

    *p++ = *first;
  }
  result.resize(p - std::begin(result));
  return result;
}

// @brief Checks if a string begins with a certain substring.
template <typename InputIt1, typename InputIt2>
bool StringStartsWith(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) {
  if (last1 - first1 < last2 - first2) {
    return false;
  }
  return std::equal(first2, last2, first1);
}

// @brief Checks if a string begins with a certain substring.
template <typename S, typename T>
bool StringStartsWith(const S& a, const T& b) {
  return StringStartsWith(a.begin(), a.end(), b.begin(), b.end());
}

// @brief Checks if a string begins with a certain substring.
template <typename T, typename CharT, size_t N>
bool StringStartsWithLiterals(const T& a, const CharT (&b)[N]) {
  return StringStartsWith(a.begin(), a.end(), b, b + N - 1);
}

/// @brief Character comparator, case-insensitive.
/// @private For internal use purpose only.
struct IgnoreCaseCharComparator {
  bool operator()(char lhs, char rhs) const {
    return std::tolower(static_cast<uint8_t>(lhs)) == std::tolower(static_cast<uint8_t>(rhs));
  }
};

// @brief Checks if a string begins with a certain substring, case-insensitive.
template <typename InputIt1, typename InputIt2>
bool StringStartsWithIgnoreCase(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) {
  if (last1 - first1 < last2 - first2) {
    return false;
  }
  return std::equal(first2, last2, first1, IgnoreCaseCharComparator());
}

// @brief Checks if a string begins with a certain substring, case-insensitive.
template <typename S, typename T>
bool StringStartsWithIgnoreCase(const S& a, const T& b) {
  return StringStartsWithIgnoreCase(a.begin(), a.end(), b.begin(), b.end());
}

// @brief Checks if a string begins with a certain substring, case-insensitive.
template <typename T, typename CharT, size_t N>
bool StringStartsWithLiteralsIgnoreCase(const T& a, const CharT (&b)[N]) {
  return StringStartsWithIgnoreCase(a.begin(), a.end(), b, b + N - 1);
}

// @brief Checks if a string ends with a certain substring.
template <typename InputIt1, typename InputIt2>
bool StringEndsWith(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) {
  if (last1 - first1 < last2 - first2) {
    return false;
  }
  return std::equal(first2, last2, last1 - (last2 - first2));
}

// @brief Checks if a string ends with a certain substring.
template <typename T, typename S>
bool StringEndsWith(const T& a, const S& b) {
  return StringEndsWith(a.begin(), a.end(), b.begin(), b.end());
}

// @brief Checks if a string ends with a certain substring.
template <typename T, typename CharT, size_t N>
bool StringEndsWithLiterals(const T& a, const CharT (&b)[N]) {
  return StringEndsWith(a.begin(), a.end(), b, b + N - 1);
}

// @brief Checks if a string ends with a certain substring, case-insensitive.
template <typename InputIt1, typename InputIt2>
bool StringEndsWithIgnoreCase(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) {
  if (last1 - first1 < last2 - first2) {
    return false;
  }
  return std::equal(first2, last2, last1 - (last2 - first2), IgnoreCaseCharComparator());
}

// @brief Checks if a string ends with a certain substring, case-insensitive.
template <typename T, typename S>
bool StringEndsWithIgnoreCase(const T& a, const S& b) {
  return StringEndsWithIgnoreCase(a.begin(), a.end(), b.begin(), b.end());
}

// @brief Checks if a string ends with a certain substring, case-insensitive.
template <typename T, typename CharT, size_t N>
bool StringEndsWithLiteralsIgnoreCase(const T& a, const CharT (&b)[N]) {
  return StringEndsWithIgnoreCase(a.begin(), a.end(), b, b + N - 1);
}

// @brief Checks if two strings are equal, case-insensitive.
template <typename InputIt1, typename InputIt2>
bool StringEqualsIgnoreCase(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) {
  if (std::distance(first1, last1) != std::distance(first2, last2)) {
    return false;
  }
  return std::equal(first1, last1, first2, IgnoreCaseCharComparator());
}

// @brief Checks if two strings are equal, case-insensitive.
template <typename T, typename S>
bool StringEqualsIgnoreCase(const T& a, const S& b) {
  return StringEqualsIgnoreCase(a.begin(), a.end(), b.begin(), b.end());
}

// @brief Checks if two strings are equal, case-insensitive.
template <typename CharT, typename InputIt, size_t N>
bool StringEqualsLiteralsIgnoreCase(const CharT (&a)[N], InputIt b, size_t blen) {
  return StringEqualsIgnoreCase(a, a + (N - 1), b, b + blen);
}

// @brief Checks if two strings are equal, case-insensitive.
template <typename CharT, size_t N, typename T>
bool StringEqualsLiteralsIgnoreCase(const CharT (&a)[N], const T& b) {
  return StringEqualsIgnoreCase(a, a + (N - 1), b.begin(), b.end());
}

// @brief Checks if two strings are equal.
template <typename InputIt1, typename InputIt2>
bool StringEquals(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) {
  if (std::distance(first1, last1) != std::distance(first2, last2)) {
    return false;
  }
  return std::equal(first1, last1, first2);
}

// @brief Checks if two strings are equal.
template <typename T, typename S>
bool StringEquals(const T& a, const S& b) {
  return StringEquals(a.begin(), a.end(), b.begin(), b.end());
}

// @brief Checks if two strings are equal.
template <typename CharT, typename InputIt, size_t N>
bool StringEqualsLiterals(const CharT (&a)[N], InputIt b, size_t blen) {
  return StringEquals(a, a + (N - 1), b, b + blen);
}

// @brief Checks if two strings are equal.
template <typename CharT, size_t N, typename T>
bool StringEqualsLiterals(const CharT (&a)[N], const T& b) {
  return StringEquals(a, a + (N - 1), b.begin(), b.end());
}
// End of source codes that are from nghttp2.

/// @brief Parses the content length from string literal value for header of "Content-Length".
std::optional<ssize_t> ParseContentLength(const char* str, size_t len);
std::optional<ssize_t> ParseContentLength(const std::string& str);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

}  // namespace trpc::http
