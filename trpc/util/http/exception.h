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

#include <exception>
#include <string>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "trpc/util/http/status.h"

namespace trpc::http {

// The following source codes are from seastar.
// Copied and modified from
// https://github.com/scylladb/seastar/blob/seastar-22.11.0/include/seastar/http/exception.hh.

/// @brief Base class of HTTP exception.
class BaseException : public std::exception {
 public:
  BaseException(std::string msg, ResponseStatus status) : msg_(std::move(msg)), status_(status) {}

  /// @brief Returns the explanatory string.
  virtual const char* what() const throw() { return msg_.c_str(); }

  /// @brief Returns status code of HTTP response status.
  ResponseStatus Status() const { return status_; }

  /// @brief Returns the explanatory string.
  virtual const std::string& str() const { return msg_; }

 private:
  std::string msg_;
  ResponseStatus status_;
};

/// @brief Throws this exception will result in a 404 error (Not Found).
class NotFoundException : public BaseException {
 public:
  explicit NotFoundException(const std::string& msg = "Not Found") : BaseException(msg, ResponseStatus::kNotFound) {}
};

/// @brief Throws this exception will result in a 504 error (Gateway Timeout).
class GatewayTimeoutException : public BaseException {
 public:
  explicit GatewayTimeoutException(const std::string& msg = "Gateway Timeout")
      : BaseException(msg, ResponseStatus::kGatewayTimeout) {}
};
using RequestTimeout = GatewayTimeoutException;

class JsonException {
 public:
  explicit JsonException(const BaseException& e) { SetParams(e.str(), e.Status()); }

  explicit JsonException(const std::exception& e) { SetParams(e.what(), ResponseStatus::kInternalServerError); }

  /// @brief Converts status and error message into JSON string.
  std::string ToJson() { return s_buffer_.GetString(); }

 private:
  void SetParams(const std::string& msg, ResponseStatus status) {
    rapidjson::Writer<rapidjson::StringBuffer> writer(s_buffer_);
    writer.StartObject();

    writer.Key("message");
    writer.String(msg.c_str(), msg.size());

    writer.Key("code");
    writer.Int(static_cast<int>(status));

    writer.EndObject();
  }

 private:
  rapidjson::StringBuffer s_buffer_;
};
// End of source codes that are from seastar.

}  // namespace trpc::http
