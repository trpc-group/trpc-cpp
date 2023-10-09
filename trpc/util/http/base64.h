//
//
// Copyright (c) 2013 Tatsuhiro Tsujikawa
// nghttp2 is licensed under the MIT License.
// The source codes in this file based on
// https://github.com/nghttp2/nghttp2/blob/v1.21.0/src/base64.h.
// This source file may have been modified by THL A29 Limited, and licensed under the MIT License.
//
//

#pragma once

#include <string>

#include "trpc/util/log/logging.h"

namespace trpc::http {

template <typename InputIt, typename OutputIt>
OutputIt Base64Encode(InputIt first, InputIt last, OutputIt d_first) {
  // clang-format off
  static constexpr char kB64Chars[] = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
      'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
      'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
  };
  // clang-format on
  size_t len = last - first;

  if (len == 0) {
    return d_first;
  }

  auto r = len % 3;
  auto j = last - r;
  auto p = d_first;
  while (first != j) {
    uint32_t n = static_cast<uint8_t>(*first++) << 16;
    n += static_cast<uint8_t>(*first++) << 8;
    n += static_cast<uint8_t>(*first++);
    *p++ = kB64Chars[n >> 18];
    *p++ = kB64Chars[(n >> 12) & 0x3fu];
    *p++ = kB64Chars[(n >> 6) & 0x3fu];
    *p++ = kB64Chars[n & 0x3fu];
  }

  if (r == 2) {
    uint32_t n = static_cast<uint8_t>(*first++) << 16;
    n += static_cast<uint8_t>(*first++) << 8;
    *p++ = kB64Chars[n >> 18];
    *p++ = kB64Chars[(n >> 12) & 0x3fu];
    *p++ = kB64Chars[(n >> 6) & 0x3fu];
    *p++ = '=';
  } else if (r == 1) {
    uint32_t n = static_cast<uint8_t>(*first++) << 16;
    *p++ = kB64Chars[n >> 18];
    *p++ = kB64Chars[(n >> 12) & 0x3fu];
    *p++ = '=';
    *p++ = '=';
  }
  return p;
}

template <typename InputIt, typename OutputIt>
OutputIt Base64Decode(InputIt first, InputIt last, OutputIt d_first) {
  // clang-format off
  static constexpr int kIndexes[] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62,
      -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0,
      1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
      23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
      39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  // clang-format on
  TRPC_ASSERT(std::distance(first, last) % 4 == 0);
  auto p = d_first;
  for (; first != last;) {
    uint32_t n = 0;
    for (int i = 1; i <= 4; ++i, ++first) {
      auto idx = kIndexes[static_cast<size_t>(*first)];
      if (idx == -1) {
        if (i <= 2) {
          return d_first;
        }
        if (i == 3) {
          if (*first == '=' && *(first + 1) == '=' && first + 2 == last) {
            *p++ = n >> 16;
            return p;
          }
          return d_first;
        }
        if (*first == '=' && first + 1 == last) {
          *p++ = n >> 16;
          *p++ = n >> 8 & 0xffu;
          return p;
        }
        return d_first;
      }

      n += idx << (24 - i * 6);
    }

    *p++ = n >> 16;
    *p++ = n >> 8 & 0xffu;
    *p++ = n & 0xffu;
  }
  return p;
}

/// @brief Base64 encoding.
/// @param first Starting address of the object to be encoded.
/// @param last Ending address of the object to be encoded.
/// @return Returns the encoded string.
template <typename InputIt>
std::string Base64Encode(InputIt first, InputIt last) {
  auto len = std::distance(first, last);
  if (len == 0) {
    return "";
  }
  std::string result;
  result.resize((len + 2) / 3 * 4);
  result.erase(Base64Encode(first, last, std::begin(result)), std::end(result));
  return result;
}

/// @brief Base64 decoding.
/// @param first Starting address of the object to be decoded.
/// @param last Ending address of the object to be decoded.
/// @return Returns the decoded string.
template <typename InputIt>
std::string Base64Decode(InputIt first, InputIt last) {
  auto len = std::distance(first, last);
  if (len % 4 != 0) {
    return "";
  }
  std::string result;
  result.resize(len / 4 * 3);
  result.erase(Base64Decode(first, last, std::begin(result)), std::end(result));
  return result;
}

}  // namespace trpc::http
