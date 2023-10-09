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

#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "nghttp2/nghttp2.h"

#include "trpc/util/buffer/buffer.h"
#include "trpc/util/http/header.h"
#include "trpc/util/http/url.h"
#include "trpc/util/http/util.h"

namespace trpc::http2 {

// @brief Callback method for closing the request or response.
// @param error_code Error code indicating the reason for the closure.
using OnClose = std::function<void(uint32_t error_code)>;

using UriRef = trpc::http::Url;
using HeaderPairs = trpc::http::HeaderPairs;
using TrailerPairs = trpc::http::TrailerPairs;

// The following source codes are from nghttp2.
// Copied and modified from
// https://github.com/nghttp2/nghttp2/blob/v1.21.1/src/http2.h.

namespace nghttp2 {
// @brief Create an nghttp2_nv (key-value pair).
//
// @param name The name.
// @param value The value.
// @param no_index Indexing flag: if true, the NGHTTP2_NV_FLAG_NO_INDEX flag will be set in nghttp_nv.flags.
//
// @return The key-value pair nghttp_nv.
// @note The returned value refers only to the name and value pointers and content length, and no copying is performed.
// The caller must ensure the content lifecycle.
// The CreatePair() series of interfaces are for internal framework use and are used by HTTP2 clients/servers.
nghttp2_nv CreatePair(std::string_view name, std::string_view value, bool no_index = false);
}  // namespace nghttp2

// @brief HTTP2 header token.
enum HeaderToken {
  // :authority
  kHeaderColonAuthority,
  // :host
  kHeaderColonHost,
  // :method
  kHeaderColonMethod,
  // :path
  kHeaderColonPath,
  // :protocol
  kHeaderColonProtocol,
  // :scheme
  kHeaderColonScheme,
  // :status
  kHeaderColonStatus,
  // accept-encoding
  kHeaderAcceptEncoding,
  // accept-language
  kHeaderAcceptLanguage,
  // alt-svc
  kHeaderAltSvc,
  // cache-control
  kHeaderCacheControl,
  // connection
  kHeaderConnection,
  // content-length
  kHeaderContentLength,
  // content-type
  kHeaderContentType,
  // cookie
  kHeaderCookie,
  // date
  kHeaderDate,
  // early-data
  kHeaderEarlyData,
  // expect
  kHeaderExpect,
  // forwarded
  kHeaderForwarded,
  // host
  kHeaderHost,
  // http2-settings
  kHeaderHttp2Settings,
  // if-modified-since
  kHeaderIfModifiedSince,
  // keep-alive
  kHeaderKeepAlive,
  // link
  kHeaderLink,
  // location
  kHeaderLocation,
  // proxy-connection
  kHeaderProxyConnection,
  // sec-websocket-accept
  kHeaderSecWebsocketAccept,
  // sec-websocket-key
  kHeaderSecWebsocketKey,
  // server
  kHeaderServer,
  // te
  kHeaderTe,
  // trailer
  kHeaderTrailer,
  // transfer-encoding
  kHeaderTransferEncoding,
  // upgrade
  kHeaderUpgrade,
  // user-agent
  kHeaderUserAgent,
  // via
  kHeaderVia,
  // x-forwarded-for
  kHeaderXForwardedFor,
  // x-forwarded-proto
  kHeaderXForwardedProto,
  // Max index.
  kHeaderMaxIndex,
};

// Field names for common headers in http2.
constexpr char kHttp2HeaderStatusName[] = ":status";
constexpr char kHttp2HeaderDateName[] = "date";

constexpr char kHttp2HeaderStatusOk[] = "200";

// Fixed header length for http2 frames.
constexpr uint8_t kHttp2FixedHeader = 9;

// @brief Look up the header name tag. If the name is an HTTP2 header.
//
// @param name Pointer to the name.
// @param name_len Length of the name.
// @return returns the corresponding index value; otherwise, return -1.
int LookupToken(const uint8_t* name, size_t name_len);

// @brief Splits URL PATH into path and query.
template <typename InputIt>
void SplitPath(InputIt first, InputIt last, UriRef* uri_ref) {
  auto path_last = std::find(first, last, '?');
  InputIt query_first;
  if (path_last == last) {
    query_first = path_last = last;
  } else {
    query_first = path_last + 1;
  }
  *(uri_ref->MutablePath()) = http::PercentDecode(first, path_last);
  uri_ref->MutableRawPath()->assign(first, path_last);
  uri_ref->MutableRawQuery()->assign(query_first, last);
}
// End of source codes that are from nghttp2.

}  // namespace trpc::http2
