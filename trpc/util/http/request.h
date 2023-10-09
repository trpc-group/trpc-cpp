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
#include <string>

#include "trpc/stream/http/http_stream.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/http/common.h"
#include "trpc/util/http/content.h"
#include "trpc/util/http/http_header.h"
#include "trpc/util/http/parameter.h"

namespace trpc::http {
// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/include/seastar/http/request.hh.

/// @brief HTTP request.
class Request : public Header {
 public:
  /// @brief The class of content-type.
  enum class ContentTypeClass : char {
    kOther,                          ///< For default content-type.
    kMultipartFormData,              ///< For content-type of "multipart/form-data".
    kApplicationXWwwFormUrlencoded,  ///< For content-type of "application/x-www-form-urlencoded".
  };

 public:
  Request(size_t queue_capacity = 0, bool blocking = false) : read_stream_(this, queue_capacity, blocking) {}

  /// @brief Reports whether the method is GET.
  bool IsGet() const { return method_type_ == MethodType::GET; }
  /// @brief Reports whether the method is HEAD.
  bool IsHead() const { return method_type_ == MethodType::HEAD; }
  /// @brief Reports whether the method is POST.
  bool IsPost() const { return method_type_ == MethodType::POST; }
  /// @brief Reports whether the method is PUT.
  bool IsPut() const { return method_type_ == MethodType::PUT; }
  /// @brief Reports whether the method is PATCH.
  bool IsPatch() const { return method_type_ == MethodType::PATCH; }
  /// @brief Reports whether the method is DELETE.
  bool IsDelete() const { return method_type_ == MethodType::DELETE; }
  /// @brief Reports whether the method is OPTIONS.
  bool IsOptions() const { return method_type_ == MethodType::OPTIONS; }

  /// @brief Gets the path parameters (Placeholder in URL path).
  const PathParameters& GetParameters() const { return path_parameters_; }
  PathParameters* GetMutableParameters() { return &path_parameters_; }

  /// @brief Sets the path parameters (Placeholder in URL path).
  void SetParameters(const PathParameters& params) { path_parameters_ = params; }
  void SetParameters(PathParameters&& params) { path_parameters_ = std::move(params); }

  /// @brief Initializes the query parameters (Query string in URL).
  /// @private
  bool InitQueryParameters();

  /// @brief Gets query parameters (Key-value pairs in URL query string).
  const QueryParameters& GetQueryParameters() const { return query_parameters_; }
  QueryParameters* GetMutableQueryParameters() { return &query_parameters_; }

  /// @brief Gets the parameter value from query parameters. Returns empty string if not found.
  std::string GetQueryParameter(const std::string& name) const;

  /// @brief Does the same job previous method, but returns default value set by user if not found.
  std::string GetQueryParameter(const std::string& name, const std::string& default_value) const;

  /// @brief Returns all key-value pairs of query parameters.
  std::vector<std::pair<std::string_view, std::string_view>> GetQueryParameterPairs() const;

  /// @brief Adds the key-value pair to query parameters.
  /// @param pair the key-value pair, key and value are joined by '=' , e.g. "name=Tom".
  void AddQueryParameter(const std::string_view& pair);

  /// @brief Sets the key-value pairs to query parameters.
  void SetQueryParameter(std::string name, std::string value);

  /// @brief Gets the class of content type.
  ContentTypeClass GetContentTypeClass() const { return content_type_class_; }

  /// @brief Sets the class of content type.
  void SetContentTypeClass(ContentTypeClass type) { content_type_class_ = type; }

  /// @brief Reports whether content is "multipart/form-data".
  bool IsMultiPart() const { return content_type_class_ == ContentTypeClass::kMultipartFormData; }

  /// @brief Reports whether content is "application/x-www-form-url-encoded".
  bool IsFormPost() const { return content_type_class_ == ContentTypeClass::kApplicationXWwwFormUrlencoded; }

  /// @brief Gets the content length.
  std::size_t ContentLength() const { return content_provider_.ContentLength(); }

  /// @brief Sets the content length.
  void SetContentLength(std::optional<size_t> content_length) { content_provider_.SetContentLength(content_length); }

  /// @brief Gets the request content (Contiguous buffer).
  const std::string& GetContent() const { return content_provider_.GetContent(); }
  std::string* GetMutableContent() { return content_provider_.GetMutableContent(); }

  /// @brief Sets the request content (Contiguous buffer).
  void SetContent(std::string content) { content_provider_.SetContent(std::move(content)); }

  /// @brief Gets the request content (Non-contiguous buffer, better performance).
  const NoncontiguousBuffer& GetNonContiguousBufferContent() const {
    return content_provider_.GetNonContiguousBufferContent();
  }
  NoncontiguousBuffer* GetMutableNonContiguousBufferContent() {
    return content_provider_.GetMutableNonContiguousBufferContent();
  }

  /// @brief Sets the request content (Non-contiguous buffer, better performance).
  void SetNonContiguousBufferContent(NoncontiguousBuffer&& content) {
    content_provider_.SetNonContiguousBufferContent(std::move(content));
  }

  /// @brief Serializes the request object into a buffer (Request line and headers and request content are included).
  std::string SerializeToString() const;
  bool SerializeToString(NoncontiguousBufferBuilder& builder) const&;
  bool SerializeToString(NoncontiguousBufferBuilder& builder) &&;
  friend std::ostream& operator<<(std::ostream& output, const Request& r);

  /// @brief Sets the max body size of request.
  /// @private For internal use purpose only.
  void SetMaxBodySize(size_t max_body_size) { max_body_size_ = max_body_size; }

  /// @brief Gets the max body size of request.
  /// @private For internal use purpose only.
  inline size_t GetMaxBodySize() { return max_body_size_; };

  /// @brief Gets the ReadStream which stores http request body.
  /// @return the ReadStream instance.
  stream::HttpReadStream& GetStream() { return read_stream_.value(); }

  /// @brief Gets the content provider.
  /// @private For internal use purpose only.
  const ContentProvider& GetContentProvider() const { return content_provider_; }
  /// @private For internal use purpose only.
  ContentProvider* GetMutableContentProvider() { return &content_provider_; }

 private:
  template <typename T>
  static bool SerializeToStringImpl(T&& self, NoncontiguousBufferBuilder& builder);

  std::string RequestLine() const;

 private:
  // Key-value pairs matched by placeholder in URL path.
  PathParameters path_parameters_;
  // Key-value pairs in URL query string.
  QueryParameters query_parameters_;
  ContentTypeClass content_type_class_;
  // For non-chunked requests, it represents the length of the body, while for chunked requests, it represents the
  // maximum allowed size of the body.
  size_t max_body_size_{0};

  // HTTP streaming.
  stream::TransientHttpReadStream read_stream_;
  ContentProvider content_provider_;
};
using RequestPtr = std::shared_ptr<Request>;
using HttpRequest = Request;
using HttpRequestPtr = std::shared_ptr<HttpRequest>;
// End of source codes that are from seastar.

}  // namespace trpc::http
