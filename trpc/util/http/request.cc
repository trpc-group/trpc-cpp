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
// 1. seastar
// Copyright (C) 2015 Cloudius Systems
// seastar is licensed under the Apache 2.0 License.
//
//

#include "trpc/util/http/request.h"

#include "trpc/util/http/util.h"

namespace trpc::http {
// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/src/http/request.cc.

bool Request::InitQueryParameters() {
  size_t pos = request_uri_.find('?');
  if (pos == std::string::npos) {
    return false;
  }
  size_t curr = pos + 1;
  size_t end_param;
  std::string_view url = request_uri_;
  while ((end_param = request_uri_.find('&', curr)) != std::string::npos) {
    AddQueryParameter(url.substr(curr, end_param - curr));
    curr = end_param + 1;
  }
  AddQueryParameter(url.substr(curr));
  return true;
}

std::string Request::GetQueryParameter(const std::string& name) const { return query_parameters_.Get(name); }

std::string Request::GetQueryParameter(const std::string& name, const std::string& default_value) const {
  auto& value = query_parameters_.Get(name);
  if (!value.empty()) {
    return value;
  }
  return default_value;
}

std::vector<std::pair<std::string_view, std::string_view>> Request::GetQueryParameterPairs() const {
  return query_parameters_.Pairs();
}

void Request::AddQueryParameter(const std::string_view& param) {
  size_t split = param.find('=');
  if (split >= param.length() - 1) {
    std::string key;
    if (UrlDecode(param.substr(0, split), key)) {
      query_parameters_.Set(std::move(key), std::string{""});
    }
  } else {
    std::string key;
    std::string value;
    if (UrlDecode(param.substr(0, split), key) && UrlDecode(param.substr(split + 1), value)) {
      query_parameters_.Set(std::move(key), std::move(value));
    }
  }
}

void Request::SetQueryParameter(std::string name, std::string value) {
  query_parameters_.Set(std::move(name), std::move(value));
}

std::string Request::SerializeToString() const {
  std::stringstream ss;
  // Request line.
  ss << RequestLine();
  // Request headers.
  headers_.WriteToStringStream(ss);
  ss << "\r\n";
  ss << content_provider_.SerializeToString();
  return ss.str();
}

bool Request::SerializeToString(NoncontiguousBufferBuilder& builder) const& {
  return SerializeToStringImpl(*this, builder);
}

bool Request::SerializeToString(NoncontiguousBufferBuilder& builder) && {
  return SerializeToStringImpl(std::move(*this), builder);
}

template <typename T>
bool Request::SerializeToStringImpl(T&& self, NoncontiguousBufferBuilder& builder) {
  // Request line.
  builder.Append(self.RequestLine());
  // Request headers.
  builder.Append(self.headers_.ToString());
  // Empty line: \r\n
  builder.Append("\r\n");
  // Request content.
  NoncontiguousBuffer buff;
  std::forward<T>(self).content_provider_.SerializeToString(buff);
  builder.Append(buff);
  return true;
}

std::string Request::RequestLine() const {
  std::string_view method{GetMethod()};
  std::string version = FullVersion();
  std::string request_line;
  // Request line format: "${method} ${request_uri} ${version}\r\n", e.g. "GET /index.html?name=Tom HTTP/1.1\r\n"
  request_line.reserve(method.size() + 1 + request_uri_.size() + 1 + version.size() + 2);
  request_line.append(method);
  request_line.append(" ");
  request_line.append(request_uri_);
  request_line.append(" ");
  request_line.append(version);
  request_line.append("\r\n");
  return request_line;
}

std::ostream& operator<<(std::ostream& output, const Request& r) { return output << r.SerializeToString(); }
// End of source codes that are from seastar.

}  // namespace trpc::http
