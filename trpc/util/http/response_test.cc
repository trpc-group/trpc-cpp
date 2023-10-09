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
//

#include "trpc/util/http/response.h"

#include "gtest/gtest.h"

namespace trpc::testing {

class ResponseTest : public ::testing::Test {
 protected:
  void SetUp() override { rsp_ = new trpc::http::HttpResponse(); }

  void TearDown() override {
    delete rsp_;
    rsp_ = nullptr;
  }

  trpc::http::Response* rsp_;
};

TEST_F(ResponseTest, AddOrSetHeader) {
  ASSERT_EQ(false, rsp_->HasHeader("Content-Length"));
  rsp_->AddHeader("Content-Length", "40");
  ASSERT_EQ(true, rsp_->HasHeader("Content-Length"));
  ASSERT_EQ("40", rsp_->GetHeader("Content-Length"));

  rsp_->SetHeader("Content-Length", "45");
  ASSERT_EQ("45", rsp_->GetHeader("Content-Length"));

  rsp_->SetHeaderIfNotPresent("Content-Length", "50");
  ASSERT_EQ("45", rsp_->GetHeader("Content-Length"));
  rsp_->AddHeader("Content-Length", "50");
  rsp_->AddHeader("Content-Length", "55");
  rsp_->DeleteHeader("Content-Length", "45");
  ASSERT_EQ("50", rsp_->GetHeader("Content-Length"));
  rsp_->DeleteHeader("Content-Length");
  ASSERT_EQ(false, rsp_->HasHeader("Content-Length"));

  rsp_->SetHeaderIfNotPresent("Content-Length", "50");
  ASSERT_EQ("50", rsp_->GetHeader("Content-Length"));

  rsp_->DeleteHeader("Content-Length", "50", std::equal_to<std::string_view>());
  ASSERT_EQ(false, rsp_->HasHeader("Content-Length"));
}

TEST_F(ResponseTest, SetVersion) {
  rsp_->SetVersion("1.1");

  ASSERT_EQ("1.1", rsp_->GetVersion());
}

TEST_F(ResponseTest, SetStatus) {
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  ASSERT_EQ(trpc::http::HttpResponse::StatusCode::kOk, rsp_->GetStatus());
  ASSERT_EQ("", rsp_->GetContent());

  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kNotModified, "not modified");
  ASSERT_EQ(trpc::http::HttpResponse::StatusCode::kNotModified, rsp_->GetStatus());
  ASSERT_EQ("not modified", rsp_->GetContent());

  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kConflict, "conflict");
  ASSERT_EQ(trpc::http::HttpResponse::StatusCode::kConflict, rsp_->GetStatus());
  ASSERT_EQ("conflict", rsp_->GetContent());

  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kTooManyRequests, "too many requests");
  ASSERT_EQ(trpc::http::HttpResponse::StatusCode::kTooManyRequests, rsp_->GetStatus());
  ASSERT_EQ("too many requests", rsp_->GetContent());
}

TEST_F(ResponseTest, SetReasonPhrase) {
  rsp_->SetReasonPhrase("unknown error");
  ASSERT_EQ("unknown error", rsp_->GetReasonPhrase());
}

TEST_F(ResponseTest, SetContent) {
  rsp_->SetContent("just test");

  ASSERT_EQ("just test", rsp_->GetContent());
}

TEST_F(ResponseTest, SetMimeType) {
  auto mime_jpg = "image/jpg";
  auto mime_json = "application/json";
  auto mime_empty = "";

  rsp_->SetMimeType(mime_jpg);

  ASSERT_EQ(mime_jpg, rsp_->GetHeader("Content-Type"));

  // It does not work to set "application/json" due to "image/jpg" has been set.
  rsp_->SetMimeType(mime_json);
  ASSERT_EQ(mime_jpg, rsp_->GetHeader("Content-Type"));

  // Sets "json" to overwrite previous value.
  rsp_->SetMimeType(mime_json, true);
  ASSERT_EQ(mime_json, rsp_->GetHeader("Content-Type"));

  rsp_->SetMimeType(mime_empty, true);
  ASSERT_EQ(mime_empty, rsp_->GetHeader("Content-Type"));

  rsp_->SetMimeType(mime_json);
  ASSERT_EQ(mime_json, rsp_->GetHeader("Content-Type"));
}

TEST_F(ResponseTest, SetContentType) {
  rsp_->SetContentType("html");

  ASSERT_EQ("text/html", rsp_->GetHeader("Content-Type"));
}

TEST_F(ResponseTest, Done) {
  rsp_->SetVersion("1.1");
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  rsp_->Done("json");

  ASSERT_EQ("application/json", rsp_->GetHeader("Content-Type"));
  ASSERT_EQ("HTTP/1.1 200 OK\r\n", rsp_->GetResponseLine());
}

TEST_F(ResponseTest, SerializeToString) {
  rsp_->SetVersion("1.1");
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  rsp_->SetContent("abc");
  rsp_->SetContentType("json");

  ASSERT_EQ(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kGatewayTimeout);
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 504 Gateway Timeout\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kTooManyRequests);
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 429 Too Many Requests\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kClientClosedRequest);
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 499 Client Closed Request\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());
}

TEST_F(ResponseTest, SerializeToStringHeaderOnly) {
  rsp_->SetVersion("1.1");
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  rsp_->SetContent("abc");
  rsp_->SetContentType("json");
  rsp_->SetHeaderOnly(true);

  ASSERT_EQ(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kGatewayTimeout);
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 504 Gateway Timeout\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kTooManyRequests);
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 429 Too Many Requests\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kClientClosedRequest);
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 499 Client Closed Request\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n",
      rsp_->SerializeToString());
}

TEST_F(ResponseTest, CheckStatusOkWithStatusOfNoContent) {
  auto status_type = trpc::http::HttpResponse::StatusCode::kNoContent;
  rsp_->SetStatus(status_type);
  rsp_->SetContent("");

  ASSERT_EQ(status_type, rsp_->GetStatus());
  ASSERT_EQ("", rsp_->GetContent());
  ASSERT_EQ(204, static_cast<int>(status_type));

  rsp_->SetVersion("1.1");
  auto status_line = rsp_->ResponseLine();
  ASSERT_EQ("HTTP/1.1 204 No Content\r\n", status_line);
}

TEST_F(ResponseTest, SetStatusWithStatusReasonPhrase) {
  rsp_->SetVersion("1.1");
  rsp_->SetStatus(800);
  rsp_->SetContent("abc");
  rsp_->SetContentType("json");

  ASSERT_EQ(
      "HTTP/1.1 800 \r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());
  int ret = rsp_->StatusCodeToStringLen();
  ASSERT_EQ(ret, strlen(" 800 \r\n"));

  rsp_->SetVersion("1.1");
  rsp_->SetStatusWithReasonPhrase(800, "User Define Reason_Phrase");
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 800 User Define Reason_Phrase\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatusWithReasonPhrase(800, "User Define Reason_Phrase\r\n");
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 800 User Define Reason_Phrase\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatusWithReasonPhrase(800, " User Define Reason_Phrase");
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 800 User Define Reason_Phrase\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatusWithReasonPhrase(800, " User Define Reason_Phrase\r\n");
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 800 User Define Reason_Phrase\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatusWithReasonPhrase(800, " ");
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 800 \r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatusWithReasonPhrase(800, "\r\n");
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 800 \r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatusWithReasonPhrase(800, " \r\n");
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 800 \r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());

  rsp_->SetVersion("1.1");
  rsp_->SetStatusWithReasonPhrase(800, "");
  rsp_->SetContent("abc");

  ASSERT_EQ(
      "HTTP/1.1 800 \r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      rsp_->SerializeToString());
}

TEST_F(ResponseTest, SerializeToOStream) {
  rsp_->SetVersion("1.1");
  rsp_->SetStatus(trpc::http::HttpResponse::StatusCode::kOk);
  rsp_->SetContent("abc");
  rsp_->SetContentType("json");

  std::stringstream ss_data;
  ss_data << *rsp_;

  ASSERT_EQ(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 3\r\n"
      "\r\n"
      "abc",
      ss_data.str());
}

}  // namespace trpc::testing
