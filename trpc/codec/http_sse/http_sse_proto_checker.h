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

#include <any>
#include <deque>

#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"

namespace trpc {

/// @brief Decodes HTTP SSE request protocol messages from binary bytes (zero-copy).
/// @param conn the connection that received the binary bytes
/// @param in the request binary bytes
/// @param out[out] a list of successfully decoded HTTP SSE requests which actual type is http::RequestPtr
/// @return the result of packet parsing:
///         kPacketLess: the request packet was not received completely
///         kPacketFull: at least one request packet has been received
///         kPacketError: parsed protocol error
int HttpSseZeroCopyCheckRequest(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out);

/// @brief Decodes HTTP SSE response protocol messages from binary bytes (zero-copy).
/// @param conn the connection that received the binary bytes
/// @param in the response binary bytes
/// @param out[out] a list of successfully decoded HTTP SSE responses which actual type is http::Response
/// @return the result of packet parsing:
///         kPacketLess: the response packet was not received completely
///         kPacketFull: at least one response packet has been received
///         kPacketError: parsed protocol error
int HttpSseZeroCopyCheckResponse(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out);

/// @brief Validates if a request is a valid SSE request
/// @param request the HTTP request to validate
/// @return true if it's a valid SSE request, false otherwise
bool IsValidSseRequest(const http::Request* request);

/// @brief Validates if a response is a valid SSE response
/// @param response the HTTP response to validate
/// @return true if it's a valid SSE response, false otherwise
bool IsValidSseResponse(const http::Response* response);

}  // namespace trpc
