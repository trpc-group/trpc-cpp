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
//

#include "trpc/codec/grpc/http2/http2.h"

#include <algorithm>
#include <utility>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(CreatePairTest, CreatePairOk) {
  std::string name{"greetings"}, value{"hello world"};
  nghttp2_nv pair = http2::nghttp2::CreatePair(name, value);
  ASSERT_EQ(name, std::string(pair.name, pair.name + pair.namelen));
  ASSERT_EQ(value, std::string(pair.value, pair.value + pair.valuelen));
  ASSERT_EQ(value, std::string(pair.value, pair.value + pair.valuelen));
  ASSERT_FALSE(pair.flags & NGHTTP2_NV_FLAG_NO_COPY_NAME);
  ASSERT_FALSE(pair.flags & NGHTTP2_NV_FLAG_NO_COPY_VALUE);
}

TEST(LookupTokenTest, LookupOk) {
  struct testing_args_t {
    std::string testing_name;
    int expect{-1};

    std::string token;
  };

  std::vector<testing_args_t> testings{
      // length: 1
      {"0", -1, "0"},
      // length: 2
      {"te", http2::kHeaderTe, "te"},
      {"0e", -1, "0e"},
      {"00", -1, "00"},
      // length: 3
      {"via", http2::kHeaderVia, "via"},
      {"00a", -1, "00a"},
      {"000", -1, "000"},
      // length: 4
      {"date", http2::kHeaderDate, "date"},
      {"000e", -1, "000e"},
      {"link", http2::kHeaderLink, "link"},
      {"000k", -1, "000k"},
      {"host", http2::kHeaderHost, "host"},
      {"000t", -1, "000t"},
      {"0000", -1, "0000"},
      // length: 5
      {":path", http2::kHeaderColonPath, ":path"},
      {"0000h", -1, "0000h"},
      {":host", http2::kHeaderColonHost, ":host"},
      {"0000t", -1, "0000t"},
      {"00000", -1, "00000"},
      // length: 6
      {"cookie", http2::kHeaderCookie, "cookie"},
      {"00000e", -1, "00000e"},
      {"server", http2::kHeaderServer, "server"},
      {"00000r", -1, "00000r"},
      {"expect", http2::kHeaderExpect, "expect"},
      {"00000t", -1, "00000t"},
      {"000000", -1, "000000"},
      // length: 7
      {"alt-svc", http2::kHeaderAltSvc, "alt-svc"},
      {"000000c", -1, "000000c"},
      {":method", http2::kHeaderColonMethod, ":method"},
      {"000000d", -1, "000000d"},
      {":scheme", http2::kHeaderColonScheme, ":scheme"},
      {"upgrade", http2::kHeaderUpgrade, "upgrade"},
      {"000000e", -1, "000000e"},
      {"trailer", http2::kHeaderTrailer, "trailer"},
      {"000000r", -1, "000000r"},
      {":status", http2::kHeaderColonStatus, ":status"},
      {"000000s", -1, "000000s"},
      {"0000000", -1, "0000000"},
      // length: 8
      {"location", http2::kHeaderLocation, "location"},
      {"0000000n", -1, "0000000n"},
      {"00000000", -1, "00000000"},
      // length: 9
      {"forwarded", http2::kHeaderForwarded, "forwarded"},
      {"00000000d", -1, "00000000d"},
      {":protocol", http2::kHeaderColonProtocol, ":protocol"},
      {"00000000l", -1, "00000000l"},
      {"000000000", -1, "000000000"},
      // length: 10
      {"early-data", http2::kHeaderEarlyData, "early-data"},
      {"000000000a", -1, "000000000a"},
      {"keep-alive", http2::kHeaderKeepAlive, "keep-alive"},
      {"000000000e", -1, "000000000e"},
      {"connection", http2::kHeaderConnection, "connection"},
      {"000000000n", -1, "000000000n"},
      {"user-agent", http2::kHeaderUserAgent, "user-agent"},
      {"000000000t", -1, "000000000t"},
      {":authority", http2::kHeaderColonAuthority, ":authority"},
      {"000000000y", -1, "000000000y"},
      {"0000000000", -1, "0000000000"},
      // length: 12
      {"content-type", http2::kHeaderContentType, "content-type"},
      {"00000000000e", -1, "00000000000e"},
      {"000000000000", -1, "000000000000"},
      // length: 13
      {"cache-control", http2::kHeaderCacheControl, "cache-control"},
      {"000000000000l", -1, "000000000000l"},
      {"0000000000000", -1, "0000000000000"},
      // length: 14
      {"content-length", http2::kHeaderContentLength, "content-length"},
      {"0000000000000h", -1, "0000000000000h"},
      {"http2-settings", http2::kHeaderHttp2Settings, "http2-settings"},
      {"0000000000000s", -1, "0000000000000s"},
      {"00000000000000", -1, "00000000000000"},
      // length: 15
      {"accept-language", http2::kHeaderAcceptLanguage, "accept-language"},
      {"00000000000000e", -1, "00000000000000e"},
      {"accept-encoding", http2::kHeaderAcceptEncoding, "accept-encoding"},
      {"00000000000000g", -1, "00000000000000g"},
      {"x-forwarded-for", http2::kHeaderXForwardedFor, "x-forwarded-for"},
      {"00000000000000r", -1, "00000000000000r"},
      {"000000000000000", -1, "000000000000000"},
      // length: 16
      {"proxy-connection", http2::kHeaderProxyConnection, "proxy-connection"},
      {"000000000000000n", -1, "000000000000000n"},
      {"0000000000000000", -1, "0000000000000000"},
      // length: 17
      {"if-modified-since", http2::kHeaderIfModifiedSince, "if-modified-since"},
      {"0000000000000000e", -1, "0000000000000000e"},
      {"transfer-encoding", http2::kHeaderTransferEncoding, "transfer-encoding"},
      {"0000000000000000g", -1, "0000000000000000g"},
      {"x-forwarded-proto", http2::kHeaderXForwardedProto, "x-forwarded-proto"},
      {"0000000000000000o", -1, "0000000000000000o"},
      {"sec-websocket-key", http2::kHeaderSecWebsocketKey, "sec-websocket-key"},
      {"0000000000000000y", -1, "0000000000000000y"},
      {"00000000000000000", -1, "00000000000000000"},
      // length: 20
      {"sec-websocket-accept", http2::kHeaderSecWebsocketAccept, "sec-websocket-accept"},
      {"0000000000000000000t", -1, "0000000000000000000t"},
      {"00000000000000000000", -1, "00000000000000000000"},

      // not exists
      {"xx-yy-zz-not-exists", -1, "xx-yy-zz-not-exists"},
  };

  for (const auto& t : testings) {
    auto name = reinterpret_cast<const uint8_t*>(t.token.c_str());
    ASSERT_EQ(t.expect, http2::LookupToken(name, t.token.length()));
  }
}

}  // namespace trpc::testing
