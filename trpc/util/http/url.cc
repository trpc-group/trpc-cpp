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
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#include "trpc/util/http/url.h"

#include <unordered_set>
#include <utility>

namespace trpc::http {

namespace {
// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/brpc/uri.cpp.

bool CheckValidChar(char c) {
  // Valid characters contains:
  // - Reserved generic: ':' , '/', '?', '#', '[', ']',  '@'.
  // - Reserved scheme- or implementation-specific: '!', '$', '&, '\'', '(', ')', '*', '+', ',' , ';' , '='.
  // - Unreserved characters: uppercase and lowercase letters, decimal digits, '-', '.', '_', '~', '%'.
  static const std::unordered_set<char> other_valid_chars{':', '/', '?', '#', '[', ']', '@', '!', '$', '&', '\'', '(',
                                                          ')', '*', '+', ',', ';', '=', '-', '.', '_', '~', '%',  ' '};
  return (std::isalnum(c) || other_valid_chars.count(c));
}
bool CheckAllSpaces(const char* begin, const char* end) {
  const char* p{begin};
  while (p < end && *p == ' ') ++p;
  return p == end;
}

void SplitHostAndPort(const char* begin, const char* end, std::string* host, std::string* port) {
  for (const char* p = end - 1; p > begin; --p) {
    if (*p >= '0' && *p <= '9') {
      continue;
    } else if (*p == ':') {
      port->assign(p + 1, end - (p + 1));
      host->assign(begin, p - begin);
      return;
    } else {
      break;
    }
  }
  host->assign(begin, end);
  return;
}

constexpr char kContinue{0};
constexpr char kCheck{1};
constexpr char kBreak{2};

inline const char FastCheck(char c) {
  // Default is continue.
  //
  // Spacial URL mark char, then check:
  // scheme + authority (scheme://userinfo@host:port):  ':', '/' , '@'
  // Space: ' '
  //
  //
  // Special URL mark char, then break:
  // path: '/',
  // query: '?'
  // fragment: '#'
  // terminate: '\0'

  // clang-format off
  static const char kX[] = {
      /* -128 ~ -1 */
      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,

      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,

      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,

      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,
      0,       0,       0,       0,       0,       0,       0,       0,

      /*  0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel */
      kBreak,       0,       0,       0,       0,       0,       0,       0,
      /*  8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si */
      0,       0,       0,       0,       0,       0,       0,       0,
      /*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
      0,       0,       0,       0,       0,       0,       0,       0,
      /*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
      0,       0,       0,       0,       0,       0,       0,       0,

      /*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
      kCheck,       0,       0,       kBreak,       0,       0,       0,       0,
      /*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
      0,       0,       0,       0,       0,       0,       0,       kBreak,
     /*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
      0,       0,       0,       0,       0,       0,       0,       0,
     /*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
      0,       0,       kCheck,       0,       0,       0,       0,       kBreak,

      /*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
      kCheck,       0,       0,       0,       0,       0,       0,       0,
      /*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
      0    ,   0    ,   0    ,   0    ,   0    ,   0    ,   0    ,   0,
      /*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
      0,       0,       0,       0,       0,       0,       0,       0,
      /*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
      0,       0,       0,       0,       0,       0,       0,       0,

      /*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
      0,       0,       0,       0,       0,       0,       0,       0,
      /* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
      0,       0,       0,       0,       0,       0,       0,       0,
      /* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
      0,       0,       0,       0,       0,       0,       0,       0,
      /* 120  x   121  y   122  z   123  {   124  ,   125  }   126  ~   127 del */
      0,       0,       0,       0,       0,       0,       0,       0,
  };
  // clang-format on
  static const char* fast_check_map = kX + 128;
  return fast_check_map[static_cast<int>(c)];
}


const char* ParseSchemeAndAuthority(const char* begin, const char* end, std::string* scheme, std::string* host,
                                    std::string* port, std::string* userinfo) {
  const char* p{begin};
  const char* start{p};
  // Checks host and port field, scheme and userinfo may be also found.
  bool got_scheme{false};
  bool got_userinfo{false};
  for (; p < end; ++p) {
    const char c = FastCheck(*p);
    if (c == kContinue) continue;
    if (c == kBreak) break;
    if (!CheckValidChar(*p)) {
      // Error: invalid character in URL.
      return nullptr;
    } else if (*p == ':') {
      // Finds scheme.
      if (p[1] == '/' && p[2] == '/' && !got_scheme) {
        scheme->assign(start, p - start);
        p += 2;
        start = p + 1;
        got_scheme = true;
      }
    } else if (*p == '@') {
      // Finds userinfo.
      if (!got_userinfo) {
        userinfo->assign(start, p - start);
        start = p + 1;
        got_userinfo = true;
      }
    } else if (*p == ' ') {
      if (!CheckAllSpaces(p + 1, end)) {
        // Error: invalid space in URL.
        return nullptr;
      }
      break;
    }
  }
  // Assigns host, and assigns port if it exists.
  SplitHostAndPort(start, p, host, port);
  return p;
}

const char* ParsePath(const char* begin, const char* end, std::string* value) {
  const char* p{begin};
  if (*p == '/') {
    const char* start = p;  // Path contains leading '/', e.g /index.html.
    ++p;
    for (; p < end && *p != '?' && *p != '#'; ++p) {
      if (*p == ' ') {
        if (!CheckAllSpaces(p + 1, end)) {
          // Error: invalid space in path.
          return nullptr;
        }
        break;
      }
    }
    // Assigns path.
    value->assign(start, p - start);
  }
  return p;
}

const char* ParseQuery(const char* begin, const char* end, std::string* value) {
  const char* p{begin};
  if (*p == '?') {
    const char* start = ++p;
    for (; *p && *p != '#'; ++p) {
      if (*p == ' ') {
        if (!CheckAllSpaces(p + 1, end)) {
          // Error: invalid space in query.
          return nullptr;
        }
        break;
      }
    }
    // Assigns query.
    value->assign(start, p - start);
  }
  return p;
}

const char* ParseFragment(const char* begin, const char* end, std::string* value) {
  const char* p{begin};
  if (*p == '#') {
    const char* start = ++p;
    for (; p < end; ++p) {
      if (*p == ' ') {
        if (!CheckAllSpaces(p + 1, end)) {
          // Error: invalid space in fragment.
          return nullptr;
        }
        break;
      }
    }
    // Assigns fragment.
    value->assign(start, p - start);
  }
  return p;
}

bool ParseUrl(const char* begin, const char* end, Url* parsed_url, std::string* err) {
  // URI = scheme ":" ["//" authority] path ["?" query] ["#" fragment]
  const char* p{begin};
  // Skips leading spaces.
  while (*p == ' ' && p < end) ++p;
  // Parses host and port field, scheme and userinfo may be also found.
  if ((p = ParseSchemeAndAuthority(p, end, parsed_url->MutableScheme(), parsed_url->MutableHost(),
                                   parsed_url->MutablePort(), parsed_url->MutableUserinfo())) == nullptr) {
    if (err) *err = "invalid character in url";
    return false;
  }
  // Parses path field.
  if ((p = ParsePath(p, end, parsed_url->MutablePath())) == nullptr) {
    if (err) *err = "invalid character in path";
    return false;
  }
  // Parses query field.
  if ((p = ParseQuery(p, end, parsed_url->MutableQuery())) == nullptr) {
    if (err) *err = "invalid character in query";
    return false;
  }
  // Parses fragment field.
  if ((p = ParseFragment(p, end, parsed_url->MutableFragment())) == nullptr) {
    if (err) *err = "invalid character in fragment";
    return false;
  }
  return true;
}
// End of source codes that are from incubator-brpc.

}  // namespace

bool ParseUrl(std::string_view url, Url* parsed_url, std::string* err) {
  return ParseUrl(url.data(), url.data() + url.size(), parsed_url, err);
}

std::string Url::Username() const {
  auto pos = userinfo_.find(':');
  if (pos == std::string::npos) {
    return userinfo_;
  }
  return userinfo_.substr(0, pos);
}

std::string Url::Password() const {
  auto pos = userinfo_.find(':');
  if (pos == std::string::npos) {
    return "";
  }
  return userinfo_.substr(pos + 1);
}

int Url::IntegerPort() const {
  if (port_.empty()) return -1;
  try {
    return std::stoi(port_);
  } catch(const std::exception&) {
    return -1;
  }
}

std::string Url::RequestUri() const {
  std::string request_uri;
  request_uri.reserve(path_.size() + query_.size() + fragment_.size() + 2);
  if (!path_.empty()) {
    request_uri.append(path_);
  }
  if (!query_.empty()) {
    request_uri.append("?");
    request_uri.append(query_);
  }
  if (!fragment_.empty()) {
    request_uri.append("#");
    request_uri.append(fragment_);
  }
  return request_uri;
}

}  // namespace trpc::http
