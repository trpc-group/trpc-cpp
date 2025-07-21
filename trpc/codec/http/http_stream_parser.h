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

#include "trpc/codec/http/http_stream_frame.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc::stream {

/// @brief Definition of error codes for HTTP packet parsing.
constexpr int kParseHttpNeedMore = -2;
constexpr int kParseHttpError = -1;
constexpr int kParseHttpSucc = 0;

/// @brief Parses a request line.
/// @param in the data to parse
/// @param [out] out request line
/// @return When successful, the number of bytes successfully parsed will be returned. When failed, kParseHttpNeedMore
///         or kParseHttpError will be returned.
int ParseHttpRequestLine(const NoncontiguousBuffer& in, HttpRequestLine* out);

/// @brief Parses a status line of a request.
/// @param in the data to parse
/// @param [out] out status line
/// @return When successful, the number of bytes successfully parsed will be returned. When failed, kParseHttpNeedMore
///         or kParseHttpError will be returned.
int ParseHttpStatusLine(const NoncontiguousBuffer& in, HttpStatusLine* out);

/// @brief Parses the header.
/// @param in the data to parse
/// @param [out] out headers
/// @param [out] meta header's metadata
/// @return When successful, the number of bytes successfully parsed will be returned. When failed, kParseHttpNeedMore
///         or kParseHttpError will be returned.
int ParseHttpHeader(const NoncontiguousBuffer& in, http::HttpHeader* out, HttpStreamHeader::MetaData* meta);

/// @brief Parses trailer
/// @param in the data to parse
/// @param [out] out trailer
/// @return When successful, the number of bytes successfully parsed will be returned. When failed, kParseHttpNeedMore
///         or kParseHttpError will be returned.
int ParseHttpTrailer(const NoncontiguousBuffer& in, http::HttpHeader* out);

/// @brief Parses a chunk
/// @param in the data to parse
/// @param [out] out chunk data
/// @return When successful, the number of bytes successfully parsed will be returned. When failed, kParseHttpNeedMore
///         or kParseHttpError will be returned.
/// @note After successfully parsing out the chunk, the chunk data in "in" will be transferred to "out", and there is no
///       need to perform a Skip operation on the "in" object externally.
int ParseHttpChunk(NoncontiguousBuffer* in, NoncontiguousBuffer* out);

}  // namespace trpc::stream
