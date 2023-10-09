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

#pragma once

#include "trpc/compressor/compressor_type.h"

// << Just for the sake of backward compatibility.
#include "trpc/util/http/method.h"
#include "trpc/util/http/parameter.h"
#include "trpc/util/http/path.h"
#include "trpc/util/http/url.h"
// >> Just for the sake of backward compatibility.

namespace trpc::http {

#ifndef DOXYGEN_SHOULD_SKIP_THIS

constexpr char kHttpPrefix[] = "HTTP/";
constexpr size_t kHttpPrefixSize{5};

constexpr char kVersion11[] = "1.1";
constexpr char kVersion10[] = "1.0";

constexpr int kMinRequestBytes{16};
constexpr int kMinResponseBytes{17};
constexpr int kMinHeaderBytes{4};

constexpr int kMaxHeaderNum{100};

constexpr char kEndOfHeaderMarker[] = "\r\n\r\n";
constexpr char kEmptyLine[] = "\r\n";

constexpr char kHeaderTransferEncoding[] = "Transfer-Encoding";
constexpr int kHeaderTransferEncodingLen{17};

constexpr char kHeaderAcceptTransferEncoding[] = "TE";
constexpr int kHeaderAcceptTransferEncodingLen{2};

constexpr char kHeaderContentLength[] = "Content-Length";
constexpr int kHeaderContentLengthLen{14};

constexpr char kHeaderContentType[] = "Content-Type";
constexpr int kHeaderContentTypeLen{12};

constexpr char kHeaderContentEncoding[] = "Content-Encoding";
constexpr int kHeaderContentEncodingLen{16};

constexpr char kHeaderAcceptEncoding[] = "Accept-Encoding";
constexpr int kHeaderAcceptEncodingLen{15};

constexpr char kHeaderConnection[] = "Connection";
constexpr char kHeaderConnectionLen{10};
constexpr char kConnectionKeepAlive[] = "keep-alive";
constexpr char kConnectionKeepAliveLen{10};
constexpr char kConnectionClose[] = "close";
constexpr char kConnectionCloseLen{5};

constexpr char kTransferEncodingChunked[] = "chunked";
constexpr char kEndOfChunkMarker[] = "\r\n";
constexpr char kEndOfChunkedResponseMarker[] = "0\r\n\r\n";
constexpr char kEndOfChunkTransferingMarker[] = "0\r\n";

constexpr char kTrailer[] = "Trailer";
constexpr int kTrailerLen{7};

/// @brief Converts 'Content-Type' to inner compression type.
/// @private For internal use purpose only.
constexpr compressor::CompressType StringToCompressType(std::string_view type) {
  // 1. Currently only supports zlib/gzip/snappy decompression, where snappy is an extension.
  // 2. Currently only supports applying one compression algorithm, does not support applying two compression
  //    algorithms in succession, and does not support applying zlib first and then applying gzip
  //    (Content-Encoding: deflate, gzip).
  if (type == "gzip") {
    return compressor::kGzip;
  } else if (type == "deflate") {
    return compressor::kZlib;
  } else if (type == "snappy") {
    return compressor::kSnappy;
  } else if (type == "identity") {
    return compressor::kNone;
  }
  return compressor::kMaxType;
}

/// @brief Converts inner compression type to text.
/// @private For internal use purpose only.
inline const std::string& CompressTypeToString(compressor::CompressType type) {
  const static std::array<std::string, compressor::kMaxType> type_names = {
      "identity",
      "gzip",
      "snappy",
      "deflate",
  };
  return type_names[type];
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

}  // namespace trpc::http
