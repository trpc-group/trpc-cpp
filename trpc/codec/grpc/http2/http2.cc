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
//

#include "trpc/codec/grpc/http2/http2.h"

#include <arpa/inet.h>

#include <algorithm>

#include "trpc/util/http/util.h"

namespace trpc::http2 {
// The following source codes are from nghttp2.
// Copied and modified from
// https://github.com/nghttp2/nghttp2/blob/v1.21.1/src/http2.cc.

namespace nghttp2 {
namespace {
nghttp2_nv CreatePairInternal(std::string_view name, std::string_view value, bool no_index,
                              uint8_t nv_flags) {
  uint8_t flags = nv_flags | (no_index ? NGHTTP2_NV_FLAG_NO_INDEX : NGHTTP2_NV_FLAG_NONE);
  return {reinterpret_cast<uint8_t*>(const_cast<char*>(name.data())),
          reinterpret_cast<uint8_t*>(const_cast<char*>(value.data())), name.size(), value.size(),
          flags};
}
}  // namespace

nghttp2_nv CreatePair(std::string_view name, std::string_view value, bool no_index) {
  return CreatePairInternal(name, value, no_index, NGHTTP2_NV_FLAG_NONE);
}

}  // namespace nghttp2

namespace {
// Reduce the cyclomatic complexity of LookupToken and split it into smaller functions.

// length: 2
int LookupTokenForLengthEqualsTwo(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'e') {
    if (http::StringEqualsLiterals("t", name, name_len - 1)) {
      return kHeaderTe;
    }
  }
  return -1;
}

// length: 3
int LookupTokenForLengthEqualsThree(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'a') {
    if (http::StringEqualsLiterals("vi", name, name_len - 1)) {
      return kHeaderVia;
    }
  }
  return -1;
}

// length: 4
int LookupTokenForLengthEqualsFour(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'e') {
    if (http::StringEqualsLiterals("dat", name, name_len - 1)) {
      return kHeaderDate;
    }
  } else if (end_char == 'k') {
    if (http::StringEqualsLiterals("lin", name, name_len - 1)) {
      return kHeaderLink;
    }
  } else if (end_char == 't') {
    if (http::StringEqualsLiterals("hos", name, name_len - 1)) {
      return kHeaderHost;
    }
  }
  return -1;
}

// length: 5
int LookupTokenForLengthEqualsFive(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'h') {
    if (http::StringEqualsLiterals(":pat", name, name_len - 1)) {
      return kHeaderColonPath;
    }
  } else if (end_char == 't') {
    if (http::StringEqualsLiterals(":hos", name, name_len - 1)) {
      return kHeaderColonHost;
    }
  }
  return -1;
}

// length: 6
int LookupTokenForLengthEqualsSix(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'e') {
    if (http::StringEqualsLiterals("cooki", name, name_len - 1)) {
      return kHeaderCookie;
    }
  } else if (end_char == 'r') {
    if (http::StringEqualsLiterals("serve", name, name_len - 1)) {
      return kHeaderServer;
    }
  } else if (end_char == 't') {
    if (http::StringEqualsLiterals("expec", name, name_len - 1)) {
      return kHeaderExpect;
    }
  }
  return -1;
}

// Reduce the cyclomatic complexity of LookupToken and split it into smaller functions.
// length: 7 and end char equals 'e'.
int LookupTokenForLengthEqualsSevenAndEndWithCharE(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'e') {
    if (http::StringEqualsLiterals(":schem", name, name_len - 1)) {
      return kHeaderColonScheme;
    } else if (http::StringEqualsLiterals("upgrad", name, name_len - 1)) {
      return kHeaderUpgrade;
    }
  }
  return -1;
}

// length: 7
int LookupTokenForLengthEqualsSeven(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'c') {
    if (http::StringEqualsLiterals("alt-sv", name, name_len - 1)) {
      return kHeaderAltSvc;
    }
  } else if (end_char == 'd') {
    if (http::StringEqualsLiterals(":metho", name, name_len - 1)) {
      return kHeaderColonMethod;
    }
  } else if (end_char == 'e') {
    return LookupTokenForLengthEqualsSevenAndEndWithCharE(name, name_len);
  } else if (end_char == 'r') {
    if (http::StringEqualsLiterals("traile", name, name_len - 1)) {
      return kHeaderTrailer;
    }
  } else if (end_char == 's') {
    if (http::StringEqualsLiterals(":statu", name, name_len - 1)) {
      return kHeaderColonStatus;
    }
  }
  return -1;
}

// length: 8
int LookupTokenForLengthEqualsEight(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'n') {
    if (http::StringEqualsLiterals("locatio", name, name_len - 1)) {
      return kHeaderLocation;
    }
  }
  return -1;
}

// length: 9
int LookupTokenForLengthEqualsNine(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'd') {
    if (http::StringEqualsLiterals("forwarde", name, name_len - 1)) {
      return kHeaderForwarded;
    }
  } else if (end_char == 'l') {
    if (http::StringEqualsLiterals(":protoco", name, name_len - 1)) {
      return kHeaderColonProtocol;
    }
  }
  return -1;
}

// length: 10
int LookupTokenForLengthEqualsTen(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'a') {
    if (http::StringEqualsLiterals("early-dat", name, name_len - 1)) {
      return kHeaderEarlyData;
    }
  } else if (end_char == 'e') {
    if (http::StringEqualsLiterals("keep-aliv", name, name_len - 1)) {
      return kHeaderKeepAlive;
    }
  } else if (end_char == 'n') {
    if (http::StringEqualsLiterals("connectio", name, name_len - 1)) {
      return kHeaderConnection;
    }
  } else if (end_char == 't') {
    if (http::StringEqualsLiterals("user-agen", name, name_len - 1)) {
      return kHeaderUserAgent;
    }
  } else if (end_char == 'y') {
    if (http::StringEqualsLiterals(":authorit", name, name_len - 1)) {
      return kHeaderColonAuthority;
    }
  }
  return -1;
}

// length: 12
int LookupTokenForLengthEqualsTwelve(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'e') {
    if (http::StringEqualsLiterals("content-typ", name, name_len - 1)) {
      return kHeaderContentType;
    }
  }
  return -1;
}

// length: 13
int LookupTokenForLengthEqualsThirteen(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'l') {
    if (http::StringEqualsLiterals("cache-contro", name, name_len - 1)) {
      return kHeaderCacheControl;
    }
  }
  return -1;
}

// length: 14
int LookupTokenForLengthEqualsFourteen(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'h') {
    if (http::StringEqualsLiterals("content-lengt", name, name_len - 1)) {
      return kHeaderContentLength;
    }
  } else if (end_char == 's') {
    if (http::StringEqualsLiterals("http2-setting", name, name_len - 1)) {
      return kHeaderHttp2Settings;
    }
  }
  return -1;
}

// length: 15
int LookupTokenForLengthEqualsFifteen(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'e') {
    if (http::StringEqualsLiterals("accept-languag", name, name_len - 1)) {
      return kHeaderAcceptLanguage;
    }
  } else if (end_char == 'g') {
    if (http::StringEqualsLiterals("accept-encodin", name, name_len - 1)) {
      return kHeaderAcceptEncoding;
    }
  } else if (end_char == 'r') {
    if (http::StringEqualsLiterals("x-forwarded-fo", name, name_len - 1)) {
      return kHeaderXForwardedFor;
    }
  }

  return -1;
}

// length: 16
int LookupTokenForLengthEqualsSixteen(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'n') {
    if (http::StringEqualsLiterals("proxy-connectio", name, name_len - 1)) {
      return kHeaderProxyConnection;
    }
  }
  return -1;
}

// length: 17
int LookupTokenForLengthEqualsSeventeen(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 'e') {
    if (http::StringEqualsLiterals("if-modified-sinc", name, name_len - 1)) {
      return kHeaderIfModifiedSince;
    }
  } else if (end_char == 'g') {
    if (http::StringEqualsLiterals("transfer-encodin", name, name_len - 1)) {
      return kHeaderTransferEncoding;
    }
  } else if (end_char == 'o') {
    if (http::StringEqualsLiterals("x-forwarded-prot", name, name_len - 1)) {
      return kHeaderXForwardedProto;
    }
  } else if (end_char == 'y') {
    if (http::StringEqualsLiterals("sec-websocket-ke", name, name_len - 1)) {
      return kHeaderSecWebsocketKey;
    }
  }
  return -1;
}

// length: 20
int LookupTokenForLengthEqualsTwenty(const uint8_t* name, size_t name_len) {
  const char end_char = name[name_len - 1];
  if (end_char == 't') {
    if (http::StringEqualsLiterals("sec-websocket-accep", name, name_len - 1)) {
      return kHeaderSecWebsocketAccept;
    }
  }
  return -1;
}

// length: < 10
int LookupTokenForLengthLessTen(const uint8_t* name, size_t name_len) {
  if (name_len == 2) {
    return LookupTokenForLengthEqualsTwo(name, name_len);
  } else if (name_len == 3) {
    return LookupTokenForLengthEqualsThree(name, name_len);
  } else if (name_len == 4) {
    return LookupTokenForLengthEqualsFour(name, name_len);
  } else if (name_len == 5) {
    return LookupTokenForLengthEqualsFive(name, name_len);
  } else if (name_len == 6) {
    return LookupTokenForLengthEqualsSix(name, name_len);
  } else if (name_len == 7) {
    return LookupTokenForLengthEqualsSeven(name, name_len);
  } else if (name_len == 8) {
    return LookupTokenForLengthEqualsEight(name, name_len);
  } else if (name_len == 9) {
    return LookupTokenForLengthEqualsNine(name, name_len);
  }
  return -1;
}
}  // namespace

// Inspired by h2o header lookup.  https://github.com/h2o/h2o
int LookupToken(const uint8_t* name, size_t name_len) {
  // Reduce the cyclomatic complexity of LookupToken and split it into smaller functions.
  if (name_len < 10) {
    return LookupTokenForLengthLessTen(name, name_len);
  } else if (name_len == 10) {
    return LookupTokenForLengthEqualsTen(name, name_len);
  } else if (name_len == 12) {
    return LookupTokenForLengthEqualsTwelve(name, name_len);
  } else if (name_len == 13) {
    return LookupTokenForLengthEqualsThirteen(name, name_len);
  } else if (name_len == 14) {
    return LookupTokenForLengthEqualsFourteen(name, name_len);
  } else if (name_len == 15) {
    return LookupTokenForLengthEqualsFifteen(name, name_len);
  } else if (name_len == 16) {
    return LookupTokenForLengthEqualsSixteen(name, name_len);
  } else if (name_len == 17) {
    return LookupTokenForLengthEqualsSeventeen(name, name_len);
  } else if (name_len == 20) {
    return LookupTokenForLengthEqualsTwenty(name, name_len);
  }
  return -1;
}
// End of source codes that are from nghttp2.

}  // namespace trpc::http2
