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
// 1. seastar
// Copyright (C) 2015 Cloudius Systems
// seastar is licensed under the Apache 2.0 License.
//
//

#include "trpc/util/http/response.h"

namespace trpc::http {
// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/src/http/reply.cc.

namespace {

static const std::unordered_map<int, std::string> kStatusString = {
    {HttpResponse::StatusCode::kContinue, " 100 Continue\r\n"},
    {HttpResponse::StatusCode::kOk, " 200 OK\r\n"},
    {HttpResponse::StatusCode::kCreated, " 201 Created\r\n"},
    {HttpResponse::StatusCode::kAccepted, " 202 Accepted\r\n"},
    {HttpResponse::StatusCode::kNoContent, " 204 No Content\r\n"},
    {HttpResponse::StatusCode::kResetContent, " 205 Reset Content\r\n"},
    {HttpResponse::StatusCode::kPartialContent, " 206 Partial Content\r\n"},
    {HttpResponse::StatusCode::kMultipleChoices, " 300 Multiple Choices\r\n"},
    {HttpResponse::StatusCode::kMovedPermanently, " 301 Moved Permanently\r\n"},
    {HttpResponse::StatusCode::kFound, " 302 Moved Temporarily\r\n"},
    {HttpResponse::StatusCode::kNotModified, " 304 Not Modified\r\n"},
    {HttpResponse::StatusCode::kBadRequest, " 400 Bad Request\r\n"},
    {HttpResponse::StatusCode::kUnauthorized, " 401 Unauthorized\r\n"},
    {HttpResponse::StatusCode::kForbidden, " 403 Forbidden\r\n"},
    {HttpResponse::StatusCode::kNotFound, " 404 Not Found\r\n"},
    {HttpResponse::StatusCode::kRequestTimeout, " 408 Request Timeout\r\n"},
    {HttpResponse::StatusCode::kConflict, " 409 Conflict\r\n"},
    {HttpResponse::StatusCode::kRequestEntityTooLarge, " 413 Request Entity Too Large\r\n"},
    {HttpResponse::StatusCode::kIAmATeapot, " 418 I'm a Teapot\r\n"},
    {HttpResponse::StatusCode::kTooManyRequests, " 429 Too Many Requests\r\n"},
    {HttpResponse::StatusCode::kClientClosedRequest, " 499 Client Closed Request\r\n"},
    {HttpResponse::StatusCode::kInternalServerError, " 500 Internal Server Error\r\n"},
    {HttpResponse::StatusCode::kNotImplemented, " 501 Not Implemented\r\n"},
    {HttpResponse::StatusCode::kBadGateway, " 502 Bad GateWay\r\n"},
    {HttpResponse::StatusCode::kServiceUnavailable, " 503 Service Unavailable\r\n"},
    {HttpResponse::StatusCode::kGatewayTimeout, " 504 Gateway Timeout\r\n"},
};

std::string ConcatStatusLine(int status, const std::string& reason_phrase) {
  std::string status_line = " " + std::to_string(status);
  // check whether reason_phrase is start with " "
  if (reason_phrase.empty() || reason_phrase[0] != ' ') {
    status_line += " ";
  }
  status_line += reason_phrase;
  // check whether reason_phrase is end with "\r\n"
  if (status_line.size() < 2 || status_line[status_line.size() - 1] != '\n' ||
      status_line[status_line.size() - 2] != '\r') {
    status_line += "\r\n";
  }
  return status_line;
}

std::string HttpDate() {
  auto t = ::time(nullptr);
  struct tm tm;
  gmtime_r(&t, &tm);
  char tmp[32];
  strftime(tmp, sizeof(tmp), "%d %b %Y %H:%M:%S GMT", &tm);
  return {tmp};
}

}  // namespace

std::string Response::StatusCodeToString() const {
  auto it = kStatusString.find(status_);
  if (it != kStatusString.end()) {
    return it->second;
  }
  return ConcatStatusLine(status_, reason_phrase_);
}

size_t Response::StatusCodeToStringLen() const { return StatusCodeToString().size(); }

std::string Response::ResponseLine() const { return StatusLine(); }

void Response::ResponseFirstLine(NoncontiguousBufferBuilder& builder) const { builder.Append(StatusLine()); }

std::string Response::HeadersToString() const {
  if (headers_.Get(http::kHeaderTransferEncoding) != http::kTransferEncodingChunked) {
    headers_.SetIfNotPresent(http::kHeaderContentLength, std::to_string(ContentLength()));
  }
  return headers_.ToString().append("\r\n");
}

std::string Response::SerializeHeaderToString() const { return StatusLine().append(HeadersToString()); }

void Response::SerializeHeaderToString(NoncontiguousBufferBuilder& builder) const {
  ResponseFirstLine(builder);
  builder.Append(headers_.ToString());
  // Empty line: "\r\n"
  builder.Append(http::kEmptyLine);
}

std::string Response::SerializeToString() const {
  std::string ss = SerializeHeaderToString();
  if (!header_only_) {
    ss.append(content_provider_.SerializeToString());
  }
  return ss;
}

bool Response::SerializeToString(NoncontiguousBuffer& buff) const& { return SerializeToStringImpl(*this, buff); }

bool Response::SerializeToString(NoncontiguousBuffer& buff) && { return SerializeToStringImpl(std::move(*this), buff); }

void Response::GenerateCommonReply(HttpRequest* req) {
  const std::string& conn = req->GetHeader(kHeaderConnection);
  if (!conn.empty()) {
    SetHeader(kHeaderConnection, conn);
  }
  SetVersion(req->GetVersion());
  SetHeader("Server", "Trpc httpd");
  SetHeader("Date", HttpDate());
}

void Response::GenerateExceptionReply(int status, const std::string& version, const std::string& content) {
  SetVersion(version);
  SetHeader(kHeaderContentType, "text/plain");
  SetHeader("Server", "Trpc httpd");
  SetHeader("Date", HttpDate());
  SetNonContiguousBufferContent(CreateBufferSlow(content));
  SetStatus(status);
}

template <typename T>
bool Response::SerializeToStringImpl(T&& self, NoncontiguousBuffer& buff) {
  // Checks whether "Transfer-Encoding" is set. This header is set and can't be overwritten when RPC over HTTP.
  if (self.headers_.Get(http::kHeaderTransferEncoding) != http::kTransferEncodingChunked) {
    self.headers_.SetIfNotPresent(http::kHeaderContentLength, std::to_string(self.ContentLength()));
  }

  NoncontiguousBufferBuilder builder;
  self.SerializeHeaderToString(builder);
  buff = builder.DestructiveGet();
  if (!self.header_only_) {
    std::forward<T>(self).content_provider_.SerializeToString(buff);
  }

  return true;
}

std::string Response::StatusLine() const {
  std::string status_line;
  status_line.reserve(32);
  status_line.append(kHttpPrefix);
  status_line.append(version_);
  status_line.append(StatusCodeToString());
  return status_line;
}

std::ostream& operator<<(std::ostream& output, const Response& r) { return output << r.SerializeToString(); }
// End of source codes that are from seastar.

}  // namespace trpc::http
