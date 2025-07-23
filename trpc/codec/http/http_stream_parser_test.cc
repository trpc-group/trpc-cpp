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

#include "trpc/codec/http/http_stream_parser.h"

#include <algorithm>

#include "gtest/gtest.h"

namespace trpc::testing {

using namespace trpc::stream;

enum ParserRetCode {
  kParserNeedMore = kParseHttpNeedMore,
  kParserError = kParseHttpError,
  kParserSucc = 0,
};

NoncontiguousBuffer BuildBuffer(std::string data) {
  NoncontiguousBufferBuilder builder;
  builder.Append(data);
  return builder.DestructiveGet();
}

TEST(HttpStreamParserTest, TestParseHttpRequestLine) {
  HttpRequestLine line;
  std::string raw_data = std::string("GET / HTTP/1.0\r\n");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), raw_data.length());
  ASSERT_EQ(line.method, "GET");
  ASSERT_EQ(line.request_uri, "/");
  ASSERT_EQ(line.version, http::kVersion10);

  raw_data = std::string("GET /hoge HTTP/1.1\r\n");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), raw_data.length());
  ASSERT_EQ(line.method, "GET");
  ASSERT_EQ(line.request_uri, "/hoge");
  ASSERT_EQ(line.version, http::kVersion11);

  // incomplete 1
  raw_data = std::string("GET");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 2
  raw_data = std::string("GET ");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 3
  raw_data = std::string("GET /");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 4
  raw_data = std::string("GET / ");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 5
  raw_data = std::string("GET / H");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 6
  raw_data = std::string("GET / HTTP/1.");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 7
  raw_data = std::string("GET / HTTP/1.0");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 8
  raw_data = std::string("GET / HTTP/1.0\r");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserNeedMore);

  // empty method
  raw_data = std::string(" / HTTP/1.0\r\nxxxxxxx");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserError);
  // empty request-target
  raw_data = std::string("GET  HTTP/1.0\r\nxxxxxxx");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserError);
  // NULL in method
  raw_data = std::string("G\0T / HTTP/1.0\r\n", 18);
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserError);
  // tab in method
  raw_data = std::string("G\tT / HTTP/1.0\r\n");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserError);
  // invalid method
  raw_data = std::string(":GET / HTTP/1.0\r\n");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserError);
  // DEL in uri-path
  raw_data = std::string("GET /\x7fhello HTTP/1.0\r\n");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserError);
  // DEL in uri-path
  raw_data = std::string("GET /\x7fhello HTTP/1.0\r\n");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), kParserError);

  // accept MSB chars
  raw_data = std::string("GET /\xa0 HTTP/1.0\r\n");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), raw_data.length());
  ASSERT_EQ(line.method, "GET");
  ASSERT_EQ(line.request_uri, "/\xa0");
  ASSERT_EQ(line.version, "1.0");

  // accept multiple spaces between tokens
  raw_data = std::string("GET   /   HTTP/1.0\r\n");
  ASSERT_EQ(ParseHttpRequestLine(BuildBuffer(raw_data), &line), raw_data.length());
  ASSERT_EQ(line.method, "GET");
  ASSERT_EQ(line.request_uri, "/");
  ASSERT_EQ(line.version, "1.0");
}

TEST(HttpStreamParserTest, TestParseHttpStatusLine) {
  HttpStatusLine line;

  std::string raw_data = std::string("HTTP/1.0 200 OK\r\n");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), raw_data.length());
  ASSERT_EQ(line.version, "1.0");
  ASSERT_EQ(line.status_code, 200);
  ASSERT_EQ(line.status_text, "OK");

  raw_data = std::string("HTTP/1.0 500 Internal Server Error\r\n");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), raw_data.length());
  ASSERT_EQ(line.version, "1.0");
  ASSERT_EQ(line.status_code, 500);
  ASSERT_EQ(line.status_text, "Internal Server Error");

  // incomplete 1
  raw_data = std::string("H");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 2
  raw_data = std::string("HTTP/1.");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 3
  raw_data = std::string("1.1");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 4
  raw_data = std::string("HTTP/1.1 ");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 5
  raw_data = std::string("HTTP/1.1 2");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 6
  raw_data = std::string("HTTP/1.1 200");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 7
  raw_data = std::string("HTTP/1.1 200 ");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 8
  raw_data = std::string("HTTP/1.1 200 O");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserNeedMore);
  // incomplete 9
  raw_data = std::string("HTTP/1.1 200 OK\r");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserNeedMore);

  // invalid http version
  raw_data = std::string("HTTP/1. 200 OK\r\nxxxxxxx");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserError);
  // invalid http version 2
  raw_data = std::string("HTTP/1.2z 200 OK\r\nxxxxxxx");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserError);
  // no status code
  raw_data = std::string("HTTP/1.1  OK\r\nxxxxxxx");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserError);
  // garbage after status 1
  raw_data = std::string("HTTP/1.1  OK\r\nxxxxxxx");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserError);
  // garbage after status 2
  raw_data = std::string("HTTP/1.1 200X \r\nxxxxxxx");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserError);
  // garbage after status 3
  raw_data = std::string("HTTP/1.1 200X OK\r\n");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), kParserError);

  // accept missing trailing whitespace in status-line
  raw_data = std::string("HTTP/1.1 200\r\nxxxxxxxx");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), 14);
  ASSERT_EQ(line.version, "1.1");
  ASSERT_EQ(line.status_code, 200);
  ASSERT_EQ(line.status_text, "");
  // accept multiple spaces between tokens
  raw_data = std::string("HTTP/1.1   200   OK\r\n");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), raw_data.length());
  ASSERT_EQ(line.version, "1.1");
  ASSERT_EQ(line.status_code, 200);
  ASSERT_EQ(line.status_text, "OK");
  // accept only \n ending
  raw_data = std::string("HTTP/1.1 200 OK\nxxxxxxx");
  ASSERT_EQ(ParseHttpStatusLine(BuildBuffer(raw_data), &line), 16);
  ASSERT_EQ(line.version, "1.1");
  ASSERT_EQ(line.status_code, 200);
  ASSERT_EQ(line.status_text, "OK");
}

TEST(HttpStreamParserTest, TestParseHttpHeader) {
  // normal
  http::HttpHeader header;
  std::string raw_data = std::string("Host: example.com\r\nCookie: \r\n\r\n");
  HttpStreamHeader::MetaData meta;
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 2);
  ASSERT_EQ(header.Get("Host"), "example.com");
  ASSERT_EQ(header.Get("Cookie"), "");
  // multibyte included
  header = http::HttpHeader();
  raw_data = std::string("Host: example.com\r\nUser-Agent: \343\201\262\343/1.0\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 2);
  ASSERT_EQ(header.Get("Host"), "example.com");
  ASSERT_EQ(header.Get("User-Agent"), "\343\201\262\343/1.0");
  // parse multiline
  header = http::HttpHeader();
  raw_data = std::string("foo: \r\nfoo: b\r\n  \tc\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 3);
  std::vector<std::string_view> vals = header.Values("foo");
  ASSERT_EQ(vals.size(), 2);
  ASSERT_TRUE(std::find(vals.begin(), vals.end(), "") != vals.end());
  ASSERT_TRUE(std::find(vals.begin(), vals.end(), "b") != vals.end());
  ASSERT_EQ(header.Get(""), "  \tc");

  // partial
  header = http::HttpHeader();
  raw_data = std::string("Host: example.com\r\nCookie: \r\n\r");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserNeedMore);

  // error
  header = http::HttpHeader();
  raw_data = std::string("Host: e\7fample.com\r\nCookie: \r\n\r");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserError);
  // parse header name with trailing space
  header = http::HttpHeader();
  raw_data = std::string("foo : ab\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserError);
  // empty header name
  header = http::HttpHeader();
  raw_data = std::string(":a\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserError);
  // header name (space only)
  header = http::HttpHeader();
  raw_data = std::string(" :a\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserError);
  // NUL in header name
  header = http::HttpHeader();
  raw_data = std::string("a\0b: c\r\n\r\n", 10);
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserError);
  // NUL in header value
  header = http::HttpHeader();
  raw_data = std::string("ab: c\0d\r\n\r\n", 11);
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserError);
  // CTL in header name
  header = http::HttpHeader();
  raw_data = std::string("a\033b: c\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserError);
  // CTL in header value
  header = http::HttpHeader();
  raw_data = std::string("ab: c\033\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserError);
  // invalid char in header value
  header = http::HttpHeader();
  raw_data = std::string("/: 1\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserError);
  // disallow {
  header = http::HttpHeader();
  raw_data = std::string("\x7b: 1\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), kParserError);

  // accept MSB chars
  header = http::HttpHeader();
  raw_data = std::string("h: c\xa2y\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 1);
  ASSERT_EQ(header.Get("h"), "c\xa2y");
  // accept |~ (though forbidden by SSE)
  header = http::HttpHeader();
  raw_data = std::string("\x7c\x7e: 1\r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 1);
  ASSERT_EQ(header.Get("\x7c\x7e"), "1");
  // exclude leading and trailing spaces in header value
  header = http::HttpHeader();
  raw_data = std::string("foo: a \t \r\n\r\n");
  ASSERT_EQ(ParseHttpHeader(BuildBuffer(raw_data), &header, &meta), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 1);
  ASSERT_EQ(header.Get("foo"), "a");
}

TEST(HttpStreamParserTest, TestParseHttpTrailer) {
  // normal
  http::HttpHeader header;
  std::string raw_data = std::string("Host: example.com\r\nCookie: \r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 2);
  ASSERT_EQ(header.Get("Host"), "example.com");
  ASSERT_EQ(header.Get("Cookie"), "");
  // multibyte included
  header = http::HttpHeader();
  raw_data = std::string("Host: example.com\r\nUser-Agent: \343\201\262\343/1.0\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 2);
  ASSERT_EQ(header.Get("Host"), "example.com");
  ASSERT_EQ(header.Get("User-Agent"), "\343\201\262\343/1.0");
  // parse multiline
  header = http::HttpHeader();
  raw_data = std::string("foo: \r\nfoo: b\r\n  \tc\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 3);
  std::vector<std::string_view> vals = header.Values("foo");
  ASSERT_EQ(vals.size(), 2);
  ASSERT_TRUE(std::find(vals.begin(), vals.end(), "") != vals.end());
  ASSERT_TRUE(std::find(vals.begin(), vals.end(), "b") != vals.end());
  ASSERT_EQ(header.Get(""), "  \tc");

  // partial
  header = http::HttpHeader();
  raw_data = std::string("Host: example.com\r\nCookie: \r\n\r");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserNeedMore);

  // error
  header = http::HttpHeader();
  raw_data = std::string("Host: e\7fample.com\r\nCookie: \r\n\r");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserError);
  // parse header name with trailing space
  header = http::HttpHeader();
  raw_data = std::string("foo : ab\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserError);
  // empty header name
  header = http::HttpHeader();
  raw_data = std::string(":a\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserError);
  // header name (space only)
  header = http::HttpHeader();
  raw_data = std::string(" :a\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserError);
  // NUL in header name
  header = http::HttpHeader();
  raw_data = std::string("a\0b: c\r\n\r\n", 10);
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserError);
  // NUL in header value
  header = http::HttpHeader();
  raw_data = std::string("ab: c\0d\r\n\r\n", 11);
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserError);
  // CTL in header name
  header = http::HttpHeader();
  raw_data = std::string("a\033b: c\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserError);
  // CTL in header value
  header = http::HttpHeader();
  raw_data = std::string("ab: c\033\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserError);
  // invalid char in header value
  header = http::HttpHeader();
  raw_data = std::string("/: 1\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserError);
  // disallow {
  header = http::HttpHeader();
  raw_data = std::string("\x7b: 1\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), kParserError);

  // accept MSB chars
  header = http::HttpHeader();
  raw_data = std::string("h: c\xa2y\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 1);
  ASSERT_EQ(header.Get("h"), "c\xa2y");
  // accept |~ (though forbidden by SSE)
  header = http::HttpHeader();
  raw_data = std::string("\x7c\x7e: 1\r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 1);
  ASSERT_EQ(header.Get("\x7c\x7e"), "1");
  // exclude leading and trailing spaces in header value
  header = http::HttpHeader();
  raw_data = std::string("foo: a \t \r\n\r\n");
  ASSERT_EQ(ParseHttpTrailer(BuildBuffer(raw_data), &header), raw_data.length());
  ASSERT_EQ(header.FlatPairsCount(), 1);
  ASSERT_EQ(header.Get("foo"), "a");
}

void DoTestParseHttpChunk(std::string raw_data, std::vector<std::string> validate_data, ParserRetCode validate_status) {
  int status = 0;
  NoncontiguousBuffer in = BuildBuffer(raw_data);
  size_t count = 0;
  int parse_bytes_count = 0;
  for (;; ++count) {
    NoncontiguousBuffer out;
    int parse_bytes = ParseHttpChunk(&in, &out);
    if (parse_bytes < 0) {
      status = parse_bytes;
      break;
    }
    parse_bytes_count += parse_bytes;
    if (out.Empty()) {
      break;
    }
    std::string chunk = FlattenSlow(out);
    ASSERT_EQ(chunk, validate_data[count]);
  }

  if (validate_status == 0) {
    ASSERT_EQ(FlattenSlow(in), "");
    ASSERT_EQ(count, validate_data.size());
    ASSERT_EQ(parse_bytes_count, raw_data.length());
  } else {
    ASSERT_EQ(status, validate_status);
  }
}

TEST(HttpStreamParserTest, TestParseHttpChunk) {
  DoTestParseHttpChunk("b\r\nhello world\r\n0\r\n", {"hello world"}, kParserSucc);
  DoTestParseHttpChunk("6\r\nhello \r\n5\r\nworld\r\n0\r\n", {"hello ", "world"}, kParserSucc);
  DoTestParseHttpChunk("6;comment=hi\r\nhello \r\n5\r\nworld\r\n0\r\n", {"hello ", "world"}, kParserSucc);
  DoTestParseHttpChunk("b\nhello world\n0\n", {"hello world"}, kParserSucc);
  // partial
  DoTestParseHttpChunk("6\r\nhello \r\nffffffffffffffff\r\nabcdefg", {"hello "}, kParserNeedMore);
  // failures
  DoTestParseHttpChunk("6\r\nhello \r\nfffffffffffffffff\r\nabcdefg", {"hello "}, kParserError);
}

}  // namespace trpc::testing
