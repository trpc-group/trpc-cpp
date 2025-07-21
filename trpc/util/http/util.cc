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
// 1. nghttp2
// Copyright (c) 2013 Tatsuhiro Tsujikawa
// nghttp2 is licensed under the MIT License.
//
// 2. seastar
// Copyright (C) 2015 Cloudius Systems
// seastar is licensed under the Apache 2.0 License.
//
//

#include "trpc/util/http/util.h"

#include <algorithm>
#include <limits>
#include <tuple>

namespace trpc::http {

// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/src/http/url.cc,
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/src/http/routes.cc.

bool UrlDecode(const std::string_view& in, std::string& out) {
  size_t pos = 0;
  auto in_size = in.size();
  out.resize(in_size);
  for (size_t i = 0; i < in_size; ++i) {
    if (in[i] == '%') {
      if (i + 3 <= in.size()) {
        out[pos++] = HexstrToChar(in, i + 1);
        i += 2;
      } else {
        return false;
      }
    } else if (in[i] == '+') {
      out[pos++] = ' ';
    } else {
      out[pos++] = in[i];
    }
  }
  out.resize(pos);
  return true;
}

char HexstrToChar(const std::string_view& in, size_t from) {
  return static_cast<char>(HexToByte(in[from]) * 16 + HexToByte(in[from + 1]));
}

char HexToByte(char c) {
  if (c >= 'a' && c <= 'z') {
    return c - 'a' + 10;
  } else if (c >= 'A' && c <= 'Z') {
    return c - 'A' + 10;
  }
  return c - '0';
}

std::string NormalizeUrl(std::string_view url) {
  if (url.length() < 2 || url.at(url.length() - 1) != '/') {
    return std::string{url};
  }
  return std::string{url.substr(0, url.length() - 1)};
}
// End of source codes that are from seastar.

// The following source codes are from nghttp2.
// Copied and modified from
// https://github.com/nghttp2/nghttp2/blob/v1.21.0/src/util.cc.

bool InRfc3986UnreservedChars(const char c) {
  // unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
  static constexpr char unreserved[] = {'-', '.', '_', '~'};

  return std::isalpha(static_cast<uint8_t>(c)) || std::isdigit(static_cast<uint8_t>(c)) ||
         std::find(std::begin(unreserved), std::end(unreserved), c) != std::end(unreserved);
}

bool InRfc3986SubDelimiters(const char c) {
  static constexpr char sub_delims[] = {'!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='};

  return std::find(std::begin(sub_delims), std::end(sub_delims), c) != std::end(sub_delims);
}

bool InToken(char c) {
  static constexpr char extra[] = {'!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~'};

  return std::isalpha(static_cast<uint8_t>(c)) || std::isdigit(static_cast<uint8_t>(c)) ||
         std::find(std::begin(extra), std::end(extra), c) != std::end(extra);
}

bool InAttrChar(char c) {
  static constexpr char bad[] = {'*', '\'', '%'};

  return InToken(c) && std::find(std::begin(bad), std::end(bad), c) == std::end(bad);
}

uint32_t HexToUint(char c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  }
  if ('A' <= c && c <= 'Z') {
    return c - 'A' + 10;
  }
  if ('a' <= c && c <= 'z') {
    return c - 'a' + 10;
  }

  return 256;
}

namespace {
std::pair<int64_t, size_t> ParseUintDigits(const void* ss, size_t len) {
  const uint8_t* s = static_cast<const uint8_t*>(ss);
  int64_t n{0};
  size_t i{0};

  if (len == 0) {
    return {-1, 0};
  }

  constexpr int64_t max = std::numeric_limits<int64_t>::max();
  constexpr int64_t max_overflow_a = max / 10;

  for (i = 0; i < len; ++i) {
    if ('0' <= s[i] && s[i] <= '9') {
      if (n > max_overflow_a) {
        return {-1, 0};
      }
      n *= 10;
      if (n > (max - (s[i] - '0'))) {
        return {-1, 0};
      }
      n += s[i] - '0';
      continue;
    }
    break;
  }

  if (i == 0) {
    return {-1, 0};
  }

  return {n, i};
}
}  // namespace

int64_t ParseUint(const uint8_t* s, size_t len) {
  int64_t n;
  size_t i;
  std::tie(n, i) = ParseUintDigits(s, len);
  if (n == -1 || i != len) {
    return -1;
  }
  return n;
}

namespace {
constexpr char kUpperXDigits[] = "0123456789ABCDEF";
}  // namespace

std::string PercentEncode(const unsigned char* target, size_t len) {
  std::string dest;
  for (size_t i = 0; i < len; ++i) {
    unsigned char c = target[i];

    if (InRfc3986UnreservedChars(c)) {
      dest += c;
    } else {
      dest += '%';
      dest += kUpperXDigits[c >> 4];
      dest += kUpperXDigits[(c & 0x0f)];
    }
  }

  return dest;
}

std::string PercentEncode(const std::string& target) {
  return PercentEncode(reinterpret_cast<const unsigned char*>(target.c_str()), target.size());
}

std::string PercentEncodePath(const std::string& s) {
  std::string dest;
  for (auto c : s) {
    if (InRfc3986UnreservedChars(c) || InRfc3986SubDelimiters(c) || c == '/') {
      dest += c;
      continue;
    }

    dest += '%';
    dest += kUpperXDigits[(c >> 4) & 0x0f];
    dest += kUpperXDigits[(c & 0x0f)];
  }
  return dest;
}

std::string PercentDecode(const std::string& s) { return PercentDecode(s.begin(), s.end()); }
// End of source codes that are from nghttp2.

std::optional<ssize_t> ParseContentLength(const char* str, size_t len) {
  char* end_ptr;
  ssize_t content_length = std::strtoll(str, &end_ptr, 10);
  return len != 0 && content_length >= 0 && static_cast<size_t>(end_ptr - str) == len
             ? std::make_optional(content_length)
             : std::nullopt;
}

std::optional<ssize_t> ParseContentLength(const std::string& str) {
  return ParseContentLength(str.c_str(), str.size());
}

}  // namespace trpc::http
