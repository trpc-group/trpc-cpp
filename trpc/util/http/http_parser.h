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

#pragma once

#include <string>
#include <deque>
#include <utility>

#include "trpc/util/http/response.h"
#include "trpc/util/http/request.h"

namespace trpc {

namespace http {

/// @brief Parses the response line and header from buf and store them in a single HttpResponse object.
/// If it is chunk encoding, the content-length field is not included in the parsed header.
/// @param buf The HTTP response message.
/// @param size The length of buf.
/// @param out The parsed HttpResponse object.
/// @return The return value represents the parsing result of buf.
/// -1: Error
/// 0: Incomplete buf
/// >0: The number of characters successfully parsed (response line + response header + blank line)
int ParseHead(const char* buf, size_t size, trpc::http::HttpResponse* out);
int ParseHead(const std::string& buf, trpc::http::HttpResponse* out);

/// @brief Parses the request line and header from buf and store them in a single HttpRequest object.
/// If it is chunk encoding, the content-length field is not included in the parsed header.
/// @param buf The HTTP request message.
/// @param size The length of buf.
/// @param out The pointer to the parsed HttpRequest object.
/// @return The return value represents the parsing result of buf.
/// -1: Error
/// 0: Incomplete buf
/// >0: The number of characters successfully parsed (request line + request header + blank line)
int ParseHead(const char* buf, size_t size, trpc::http::HttpRequest* out);
int ParseHead(const std::string& buf, trpc::http::HttpRequest* out);

/// @brief Parses one or more HttpResponse objects from buf.
/// @param buf The HTTP response message.
/// @param size The length of buf.
/// @param out The pointer to the deque of parsed HttpResponse objects.
/// @return The return value represents the parsing result of buf.
/// -1: Error
/// 0: Incomplete buf
/// >0: The number of characters successfully parsed.
int Parse(const char* buf, size_t size, std::deque<trpc::http::HttpResponse>* out);
int Parse(const std::string& buf, std::deque<trpc::http::HttpResponse>* out);

/// @brief Parses a single HttpResponse object from buf.
/// @param buf The HTTP response message.
/// @param size The length of buf.
/// @param out The pointer to the parsed HttpResponse object.
/// @return The return value represents the parsing result of buf.
/// -1: Error
/// 0: Incomplete buf
/// >0: The number of characters successfully parsed.
int Parse(const char* buf, size_t size, trpc::http::HttpResponse* out);
int Parse(const std::string& buf, trpc::http::HttpResponse* out);

/// @brief Parses one or more HttpRequest objects from buf.
/// @param buf The HTTP request message.
/// @param size The length of buf.
/// @param out The pointer to the deque of parsed HttpRequest objects.
/// @return The return value represents the parsing result of buf.
/// -1: Error
/// 0: Incomplete buf
/// >0: The number of characters successfully parsed.
int Parse(const char* buf, size_t size, std::deque<trpc::http::HttpRequest>* out);
int Parse(const std::string& buf, std::deque<trpc::http::HttpRequest>* out);

/// @brief Parses a single HttpRequest object from buf.
/// @param buf The HTTP request message.
/// @param size The length of buf.
/// @param out The pointer to the parsed HttpRequest object.
/// @return The return value represents the parsing result of buf.
/// -1: Error
/// 0: Incomplete buf
/// >0: The number of characters successfully parsed.
int Parse(const char* buf, size_t size, trpc::http::HttpRequest* out);
int Parse(const std::string& buf, trpc::http::HttpRequest* out);

}  // namespace http

}  // namespace trpc
