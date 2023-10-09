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

#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "trpc/stream/http/http_stream.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/http/common.h"
#include "trpc/util/http/content.h"
#include "trpc/util/http/http_header.h"
#include "trpc/util/http/mime_types.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/status.h"

namespace trpc::http {
// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/include/seastar/http/reply.hh

/// @brief HTTP response.
class Response : public Header {
 public:
  using StatusCode = ResponseStatus;

 public:
  Response() = default;

  /// @brief Reports whether status code is 200.
  bool IsOK() const { return status_ == ResponseStatus::kOk; }

  /// @brief Gets the status.
  int GetStatus() const { return status_; }

  /// @brief Sets the status.
  void SetStatus(int status) { status_ = status; }

  /// @brief Sets the status and response content, it's easy to set simple response.
  void SetStatus(int status, std::string content) {
    status_ = status;
    content_provider_.SetContent(std::move(content));
  }

  /// @brief Gets the user-defined reason phrase.
  const std::string& GetReasonPhrase() const { return reason_phrase_; }

  /// @brief Sets the user-defined reason phrase.
  void SetReasonPhrase(std::string reason_phrase) { reason_phrase_ = std::move(reason_phrase); }

  /// @brief Sets status with user-defined reason phrase.
  void SetStatusWithReasonPhrase(int status, std::string reason_phrase) {
    status_ = status;
    reason_phrase_ = std::move(reason_phrase);
  }

  /// @brief Gets formatted string of status which contains "${status} ${reason_phrase}", e.g. "200 OK".
  std::string FullStatus() const {
    std::string str{std::to_string(status_)};
    std::string_view reason_phrase{StatusReasonPhrase(status_)};
    if (reason_phrase.empty()) {
      reason_phrase = reason_phrase_;
    }
    str.reserve(str.size() + 1 + reason_phrase.size());
    // Checks whether reason_phrase starts with " ".
    if (reason_phrase.empty() || reason_phrase[0] != ' ') {
      str.append(" ");
    }
    str.append(reason_phrase);
    return str;
  }

  /// @brief Gets the content length.
  std::size_t ContentLength() const { return content_provider_.ContentLength(); }

  /// @brief Sets the content length.
  void SetContentLength(std::optional<size_t> content_length) { content_provider_.SetContentLength(content_length); }

  /// @brief Gets the response content (contiguous buffer).
  const std::string& GetContent() const { return content_provider_.GetContent(); }
  std::string* GetMutableContent() { return content_provider_.GetMutableContent(); }

  /// @brief Sets the response content (contiguous buffer).
  void SetContent(std::string content) { content_provider_.SetContent(std::move(content)); }

  /// @brief Gets the response content (non-contiguous buffer, better performance).
  const NoncontiguousBuffer& GetNonContiguousBufferContent() const {
    return content_provider_.GetNonContiguousBufferContent();
  }
  NoncontiguousBuffer* GetMutableNonContiguousBufferContent() {
    return content_provider_.GetMutableNonContiguousBufferContent();
  }

  /// @brief Sets the response content (non-contiguous buffer, better performance).
  void SetNonContiguousBufferContent(NoncontiguousBuffer&& content) {
    content_provider_.SetNonContiguousBufferContent(std::move(content));
  }

  /// @brief Gets the status line.
  /// @private For internal use purpose only.
  const std::string& GetResponseLine() const { return response_line_; }

  /// @brief Sets the status line.
  /// @private For internal use purpose only.
  void SetResponseLine(std::string response_line) { response_line_ = std::move(response_line); }

  /// @brief Sets the mime type.
  void SetMimeType(std::string mime, bool overwrite = false) {
    if (overwrite || headers_.Get(http::kHeaderContentType).empty()) {
      headers_.Set(http::kHeaderContentType, std::move(mime));
    }
  }

  /// @brief Sets the content type.
  void SetContentType(const std::string& content_type = "html") { SetMimeType(ExtensionToType(content_type)); }

  /// @brief Finished the response with content-type.
  /// @private For internal use purpose only.
  void Done(const std::string& content_type) {
    SetContentType(content_type);
    Done();
  }

  /// @brief Finished the response.
  /// @private For internal use purpose only.
  void Done() { response_line_ = ResponseLine(); }

  /// @brief Converts status code to string.
  std::string StatusCodeToString() const;

  /// @brief Returns the size of status string.
  size_t StatusCodeToStringLen() const;

  /// @brief Returns status line string of response.
  std::string ResponseLine() const;

  /// @brief Writes status line string of response to buffer.
  void ResponseFirstLine(NoncontiguousBufferBuilder& builder) const;

  /// @brief Converts headers of response to string.
  std::string HeadersToString() const;

  /// @brief Serializes header of response (Status line and headers are included).
  std::string SerializeHeaderToString() const;
  void SerializeHeaderToString(NoncontiguousBufferBuilder& builder) const;

  /// @brief Serializes the response object into a buffer (status line and headers and response content are included).
  std::string SerializeToString() const;
  bool SerializeToString(NoncontiguousBuffer& buff) const&;
  bool SerializeToString(NoncontiguousBuffer& buff) &&;
  friend std::ostream& operator<<(std::ostream& output, const Response& r);

  /// @private For internal use purpose only.
  void GenerateCommonReply(Request* req);
  /// @private For internal use purpose only.
  void GenerateExceptionReply(int status, const std::string& version, const std::string& content);

  /// @brief Enables the stream message writer.
  /// @param context the context for this call
  /// @private For internal use purpose only.
  void EnableStream(ServerContext* context) { write_stream_.emplace(this, context); }

  /// @brief Gets the WriteStream for sending response.
  /// @return the WriteStream instance.
  stream::HttpWriteStream& GetStream() { return write_stream_.value(); }

  /// @brief Gets the content provider.
  /// @private For internal use purpose only.
  const ContentProvider& GetContentProvider() const { return content_provider_; }
  /// @private For internal use purpose only.
  ContentProvider* GetMutableContentProvider() { return &content_provider_; }

  /// @brief Gets the trailer of HTTP Protocol.
  const TrailerPairs& GetTrailer() const { return trailers_; }
  TrailerPairs* GetMutableTrailer() { return &trailers_; }

  /// @brief Reports whether the response is header only.
  /// @private For internal use purpose only.
  bool IsHeaderOnly() const { return header_only_; }

  /// @brief Sets whether the response is header only.
  /// @private For internal use purpose only.
  void SetHeaderOnly(bool header_only) { header_only_ = header_only; }

  /// @brief Gets or sets whether the connection is reusable.
  /// @private For internal use purpose only.
  bool IsConnectionReusable() const { return connection_reusable_; }
  /// @private For internal use purpose only.
  void SetConnectionReusable(bool connection_reusable) { connection_reusable_ = connection_reusable; }

 private:
  size_t GetResponseLen() const;

  template <typename T>
  static bool SerializeToStringImpl(T&& self, NoncontiguousBuffer& buff);

  // @brief Returns status line of response.
  // Status line format: "${version} ${status} ${reason_phrase}\r\n", e.g. "HTTP/1.1 200 OK\r\n"
  std::string StatusLine() const;

 private:
  // Response status.
  int status_{ResponseStatus::kOk};
  // It's usually an empty string, It works if user wants set user-defined reason phrase.
  std::string reason_phrase_;
  // Status line, e.g. HTTP/1.1 200 OK
  std::string response_line_;
  // HTTP Streaming.
  stream::TransientHttpWriteStream write_stream_;
  bool header_only_{false};
  bool connection_reusable_{false};
  ContentProvider content_provider_;
  TrailerPairs trailers_;

};
using ResponsePtr = std::shared_ptr<Response>;
using HttpResponse = Response;
using HttpReply = HttpResponse;
using HttpResponsePtr = std::shared_ptr<HttpResponse>;
// End of source codes that are from seastar.

}  // namespace trpc::http
