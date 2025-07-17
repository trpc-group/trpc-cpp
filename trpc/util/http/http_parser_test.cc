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

#include "trpc/util/http/http_parser.h"

#include <gtest/gtest.h>

namespace trpc {

namespace testing {

class HttpParserTest : public ::testing::Test {
 protected:
  std::deque<trpc::http::HttpResponse> http_reply_out_;
  std::deque<trpc::http::HttpRequest> http_request_out_;
};

TEST_F(HttpParserTest, EmptyBufferReply) {
  std::string s = "";

  int ret = trpc::http::Parse(s, &http_reply_out_);

  EXPECT_EQ(ret, 0);
  EXPECT_EQ(http_reply_out_.size(), 0);

  trpc::http::HttpResponse rsp;
  ret = trpc::http::Parse(s, &rsp);

  EXPECT_EQ(ret, 0);
}

TEST_F(HttpParserTest, SingleFullBufferReply) {
  std::string s = "HTTP/1.1 200 OK\r\nContent-Length:6\r\n\r\n123456";

  int ret = trpc::http::Parse(s, &http_reply_out_);

  EXPECT_EQ(ret, s.size());
  EXPECT_EQ(http_reply_out_.size(), 1);

  trpc::http::HttpResponse& reply = http_reply_out_.front();

  EXPECT_EQ(reply.GetVersion(), "1.1");
  EXPECT_EQ(reply.GetStatus(), trpc::http::HttpResponse::StatusCode::kOk);
  EXPECT_EQ(reply.GetReasonPhrase(), "OK");
  EXPECT_EQ(reply.GetContent(), "123456");
  EXPECT_EQ(reply.GetHeader("Content-Length"), "6");

  trpc::http::HttpResponse rsp;
  ret = trpc::http::ParseHead(s, &rsp);

  EXPECT_EQ(ret, s.size() - 6);
  EXPECT_EQ(rsp.GetVersion(), "1.1");
  EXPECT_EQ(rsp.GetStatus(), trpc::http::HttpResponse::StatusCode::kOk);
  EXPECT_EQ(rsp.GetReasonPhrase(), "OK");

  ret = trpc::http::Parse(s, &rsp);

  EXPECT_EQ(ret, s.size());
  EXPECT_EQ(rsp.GetContent(), "123456");
  EXPECT_EQ(rsp.GetHeader("Content-Length"), "6");
}

TEST_F(HttpParserTest, SingleLessBufferReply) {
  std::string s = "HTTP/1.1 200 OK\r\nContent-Length:6\r\n\r\n1234";

  int ret = trpc::http::Parse(s, &http_reply_out_);

  EXPECT_EQ(ret, 0);
  EXPECT_EQ(http_reply_out_.size(), 0);

  trpc::http::HttpResponse rsp;
  ret = trpc::http::Parse(s, &rsp);

  EXPECT_EQ(ret, 0);

  s = s.substr(0, 20);
  ret = trpc::http::Parse(s, &http_reply_out_);

  EXPECT_EQ(ret, 0);
  EXPECT_EQ(http_reply_out_.size(), 0);

  ret = trpc::http::Parse(s, &rsp);

  EXPECT_EQ(ret, 0);
}

TEST_F(HttpParserTest, SingleBadBuffer) {
  std::string s = "HTTP/1.1 2001 OK\r\nContent-Length:6\r\n\r\n1234";

  int ret = trpc::http::Parse(s, &http_reply_out_);

  EXPECT_EQ(ret, -1);
  EXPECT_EQ(http_reply_out_.size(), 0);

  trpc::http::HttpResponse rsp;
  ret = trpc::http::Parse(s, &rsp);

  EXPECT_EQ(ret, -1);
}

TEST_F(HttpParserTest, MultiFullBufferReply) {
  const char* single_buffer = "HTTP/1.0 200 OK\r\nContent-Length:5\r\n\r\n12345";
  std::string s;
  for (int i = 0; i < 3; ++i) {
    s.append(single_buffer);
  }

  int ret = trpc::http::Parse(s, &http_reply_out_);

  EXPECT_EQ(ret, s.size());
  EXPECT_EQ(http_reply_out_.size(), 3);

  trpc::http::HttpResponse& reply = http_reply_out_[1];

  EXPECT_EQ(reply.GetVersion(), "1.0");
  EXPECT_EQ(reply.GetStatus(), trpc::http::HttpResponse::StatusCode::kOk);
  EXPECT_EQ(reply.GetReasonPhrase(), "OK");
  EXPECT_EQ(reply.GetContent(), "12345");
  EXPECT_EQ(reply.GetHeader("Content-Length"), "5");
}

TEST_F(HttpParserTest, MultiNotFullBufferReply) {
  const char* single_buffer = "HTTP/1.1 404 Not Found\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length:5\r\n\r\n"
                              "12345";
  std::string s;
  for (int i = 0; i < 2; ++i) {
    s.append(single_buffer);
  }

  s.append(single_buffer, 11);

  int ret = trpc::http::Parse(s, &http_reply_out_);

  EXPECT_EQ(ret, s.size() - 11);
  EXPECT_EQ(http_reply_out_.size(), 2);

  trpc::http::HttpResponse& reply = http_reply_out_[1];

  EXPECT_EQ(reply.GetVersion(), "1.1");
  EXPECT_EQ(reply.GetStatus(), trpc::http::HttpResponse::StatusCode::kNotFound);
  EXPECT_EQ(reply.GetReasonPhrase(), "Not Found");
  EXPECT_EQ(reply.GetContent(), "12345");
  EXPECT_EQ(reply.GetHeader("Content-Length"), "5");
  EXPECT_EQ(reply.GetHeader("Content-Type"), "application/json");
}

TEST_F(HttpParserTest, ChunkedReply) {
  std::string s = "HTTP/1.1 200 OK\r\nTransfer-Encoding:chunked\r\n\r\n"
                  "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";

  int ret = trpc::http::Parse(s, &http_reply_out_);

  EXPECT_EQ(ret, s.size());
  EXPECT_EQ(http_reply_out_.size(), 1);

  trpc::http::HttpResponse& reply = http_reply_out_.front();
  auto found = reply.GetHeader(trpc::http::kHeaderTransferEncoding);
  EXPECT_TRUE(found.empty());

  found = reply.GetHeader(trpc::http::kHeaderContentLength);
  EXPECT_TRUE(!found.empty());

  EXPECT_EQ(reply.GetVersion(), "1.1");
  EXPECT_EQ(reply.GetStatus(), trpc::http::HttpResponse::StatusCode::kOk);
  EXPECT_EQ(reply.GetReasonPhrase(), "OK");
  EXPECT_EQ(reply.GetContent(), "helloworld");
  EXPECT_EQ(reply.GetHeader("Content-Length"), "10");
}

TEST_F(HttpParserTest, MutiFullChunkedReply) {
  const char* single_buffer = "HTTP/1.1 200 OK\r\nTransfer-Encoding:chunked\r\n\r\n"
      "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";
  std::string s;
  for (int i = 0; i < 3; ++i) {
    s.append(single_buffer);
  }

  int ret = trpc::http::Parse(s, &http_reply_out_);

  EXPECT_EQ(ret, s.size());
  EXPECT_EQ(http_reply_out_.size(), 3);

  trpc::http::HttpResponse& reply = http_reply_out_.front();

  EXPECT_EQ(reply.GetVersion(), "1.1");
  EXPECT_EQ(reply.GetStatus(), trpc::http::HttpResponse::StatusCode::kOk);
  EXPECT_EQ(reply.GetReasonPhrase(), "OK");
  EXPECT_EQ(reply.GetContent(), "helloworld");
  EXPECT_EQ(reply.GetHeader("Content-Length"), "10");
}

TEST_F(HttpParserTest, MutiNotFullChunkedReply) {
  const char* single_buffer = "HTTP/1.1 200 OK\r\nTransfer-Encoding:chunked\r\n\r\n"
                              "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";
  std::string s;
  for (int i = 0; i < 3; ++i) {
    s.append(single_buffer);
  }
  s.append(single_buffer, 55);

  int ret = trpc::http::Parse(s, &http_reply_out_);

  EXPECT_EQ(ret, s.size() - 55);
  EXPECT_EQ(http_reply_out_.size(), 3);

  trpc::http::HttpResponse& reply = http_reply_out_[1];

  EXPECT_EQ(reply.GetVersion(), "1.1");
  EXPECT_EQ(reply.GetStatus(), trpc::http::HttpResponse::StatusCode::kOk);
  EXPECT_EQ(reply.GetReasonPhrase(), "OK");
  EXPECT_EQ(reply.GetContent(), "helloworld");
  EXPECT_EQ(reply.GetHeader("Content-Length"), "10");
}

TEST_F(HttpParserTest, EmptyBufferRequest) {
  std::string s = "";

  int ret = trpc::http::Parse(s, &http_request_out_);

  EXPECT_EQ(ret, 0);
  EXPECT_EQ(http_reply_out_.size(), 0);

  trpc::http::HttpRequest req;
  ret = trpc::http::Parse(s, &req);

  EXPECT_EQ(ret, 0);
}

TEST_F(HttpParserTest, SingleFullRequest) {
  std::string s = "GET /form.html HTTP/1.1\r\nAccept-Language:zh-cn\r\n\r\n";

  int ret = trpc::http::Parse(s, &http_request_out_);

  ASSERT_EQ(ret, s.size());
  ASSERT_EQ(http_request_out_.size(), 1);

  auto& req = http_request_out_.front();

  EXPECT_EQ(req.GetVersion(), http::kVersion11);
  EXPECT_EQ(req.GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(*req.GetMutableUrl(), "/form.html");
  EXPECT_EQ(req.GetContent(), "");
  EXPECT_EQ(req.GetHeader("Content-Length"), "0");

  trpc::http::HttpRequest request;
  ret = trpc::http::ParseHead(s, &request);
  EXPECT_EQ(request.GetVersion(), http::kVersion11);
  EXPECT_EQ(request.GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(*request.GetMutableUrl(), "/form.html");
  EXPECT_EQ(ret, s.size());

  ret = trpc::http::Parse(s, &request);

  EXPECT_EQ(ret, s.size());
  EXPECT_EQ(request.GetContent(), "");
  EXPECT_EQ(request.GetHeader("Content-Length"), "0");
}

TEST_F(HttpParserTest, SetGetContentType) {
  std::string s = "GET /form.html HTTP/1.1\r\ncontent-type:text/html\r\n"
                  "Accept-Language:zh-cn\r\n\r\n";

  int ret = trpc::http::Parse(s, &http_request_out_);

  ASSERT_EQ(ret, s.size());
  ASSERT_EQ(http_request_out_.size(), 1);

  auto& req = http_request_out_.front();
  ASSERT_EQ(req.GetHeader("Content-Type"), "text/html");
}

TEST_F(HttpParserTest, SetGetContentTypeMore) {
  std::string s = "GET /form.html HTTP/1.1\r\ncontent-type:text/html; charset=utf-8\r\n"
                  "Accept-Language:zh-cn\r\n\r\n";

  int ret = trpc::http::Parse(s, &http_request_out_);

  ASSERT_EQ(ret, s.size());
  ASSERT_EQ(http_request_out_.size(), 1);

  auto& req = http_request_out_.front();
  ASSERT_EQ(req.GetHeader("Content-Type"), "text/html; charset=utf-8");
}

TEST_F(HttpParserTest, SingleErrorBufferRequest) {
  std::string s = "GET /form.html HTTP/1.1\rAccept-Language:zh-cn\r\n\r\n";

  int ret = trpc::http::Parse(s, &http_request_out_);

  ASSERT_EQ(ret, -1);
  ASSERT_EQ(http_request_out_.size(), 0);
}
TEST_F(HttpParserTest, SingleNotFullBufferRequest) {
  std::string s = "GET /form.html HTTP/1.1\r\n"
                  "Content-Length:10\r\n"
                  "content-type:text/html\r\n"
                  "Accept-Language:zh-cn\r\n"
                  "hellowo";

  int ret = trpc::http::Parse(s, &http_request_out_);

  ASSERT_EQ(ret, 0);
  ASSERT_EQ(http_request_out_.size(), 0);
}

TEST_F(HttpParserTest, MultiFullBufferRequest) {
  const char* single_buffer = "GET /form.html HTTP/1.0\r\nAccept-Language:zh-cn\r\n\r\n";
  std::string s;

  for (int i = 0; i < 3; ++i) {
    s.append(single_buffer);
  }

  int ret = trpc::http::Parse(s, &http_request_out_);

  ASSERT_EQ(ret, s.size());
  ASSERT_EQ(http_request_out_.size(), 3);

  auto& req = http_request_out_.front();

  EXPECT_EQ(req.GetVersion(), http::kVersion10);
  EXPECT_EQ(req.GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(*req.GetMutableUrl(), "/form.html");
  EXPECT_EQ(req.GetContent(), "");
  EXPECT_EQ(req.GetHeader("Content-Length"), "0");
  EXPECT_EQ(req.GetHeader("Accept-Language"), "zh-cn");
}

TEST_F(HttpParserTest, MultiNotFullBufferRequest) {
  const char* single_buffer = "GET /form.html HTTP/1.1\r\n"
                              "Content-Length:10\r\n"
                              "content-type:text/html\r\n"
                              "Accept-Language:zh-cn\r\n"
                              "Host:127.0.0.1\r\n\r\n"
                              "helloworld";
  std::string s;

  for (int i = 0; i < 3; ++i) {
    s.append(single_buffer);
  }

  s.append(s.c_str(), 10);

  int ret = trpc::http::Parse(s, &http_request_out_);

  ASSERT_EQ(ret, s.size() - 10);
  ASSERT_EQ(http_request_out_.size(), 3);

  auto& req = http_request_out_.back();

  EXPECT_EQ(req.GetVersion(), http::kVersion11);
  EXPECT_EQ(req.GetMethodType(), http::OperationType::GET);
  EXPECT_EQ(*req.GetMutableUrl(), "/form.html");
  EXPECT_EQ(req.GetContent(), "helloworld");
  EXPECT_EQ(req.GetHeader("Content-Length"), "10");
  EXPECT_EQ(req.GetHeader("Accept-Language"), "zh-cn");
  EXPECT_EQ(req.GetHeader("Host"), "127.0.0.1");
}

TEST_F(HttpParserTest, ChunkedRequest) {
  std::string s = "POST /form.html HTTP/1.1\r\nTransfer-Encoding:chunked\r\n\r\n"
                  "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";

  int ret = trpc::http::Parse(s, &http_request_out_);

  ASSERT_EQ(ret, s.size());
  ASSERT_EQ(http_request_out_.size(), 1);

  auto& req = http_request_out_.front();
  auto found = req.GetHeader(http::kHeaderTransferEncoding);
  EXPECT_TRUE(found.empty());

  found = req.GetHeader(http::kHeaderContentLength);
  EXPECT_TRUE(!found.empty());

  EXPECT_EQ(req.GetVersion(), http::kVersion11);
  EXPECT_EQ(req.GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(*req.GetMutableUrl(), "/form.html");
  EXPECT_EQ(req.GetContent(), "helloworld");
  EXPECT_EQ(req.GetHeader("Content-Length"), "10");
}

TEST_F(HttpParserTest, MutiChunkedRequest) {
  const char* single_buffer = "POST /form.html HTTP/1.1\r\nTransfer-Encoding:chunked\r\n\r\n"
                              "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";
  std::string s;
  for (int i = 0; i < 3; ++i) {
    s.append(single_buffer);
  }
  int ret = trpc::http::Parse(s, &http_request_out_);

  ASSERT_EQ(ret, s.size());
  ASSERT_EQ(http_request_out_.size(), 3);

  auto& req = http_request_out_[1];

  EXPECT_EQ(req.GetVersion(), http::kVersion11);
  EXPECT_EQ(req.GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(*req.GetMutableUrl(), "/form.html");
  EXPECT_EQ(req.GetContent(), "helloworld");
  EXPECT_EQ(req.GetHeader("Content-Length"), "10");
}

TEST_F(HttpParserTest, MutiNotFullChunkedRequest) {
  const char* single_buffer = "POST /form.html HTTP/1.1\r\nTransfer-Encoding:chunked\r\n\r\n"
                              "5\r\nhello\r\n5\r\nworld\r\n0\r\n\r\n";
  std::string s;
  for (int i = 0; i < 3; ++i) {
    s.append(single_buffer);
  }
  s.append(single_buffer, 60);

  int ret = trpc::http::Parse(s, &http_request_out_);

  ASSERT_EQ(ret, s.size() - 60);
  ASSERT_EQ(http_request_out_.size(), 3);

  auto& req = http_request_out_[1];

  EXPECT_EQ(req.GetVersion(), http::kVersion11);
  EXPECT_EQ(req.GetMethodType(), http::OperationType::POST);
  EXPECT_EQ(*req.GetMutableUrl(), "/form.html");
  EXPECT_EQ(req.GetContent(), "helloworld");
  EXPECT_EQ(req.GetHeader("Content-Length"), "10");
}

}  // namespace testing

}  // namespace trpc
