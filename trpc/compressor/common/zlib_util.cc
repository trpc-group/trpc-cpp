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

#include "trpc/compressor/common/zlib_util.h"

#include <algorithm>

#include "zlib.h"

#include "trpc/util/buffer/zero_copy_stream.h"
#include "trpc/util/log/logging.h"

namespace trpc::compressor::zlib {

int ConvertLevel(LevelType level) {
  switch (level) {
    case kFastest:
      return Z_BEST_SPEED;
    case kBest:
      return Z_BEST_COMPRESSION;
    default:
      return Z_DEFAULT_COMPRESSION;
  }
}

int GetWindowBits(CompressType type) {
  switch (type) {
    case kGzip:
      return MAX_WBITS + 16;
    default:
      return MAX_WBITS;
  }
}

// Reference: http://www.zlib.net/zlib_how.html
bool Compress(CompressType type, const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level) {
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  int ret = deflateInit2(&strm, ConvertLevel(level), Z_DEFLATED, GetWindowBits(type), 8, Z_DEFAULT_STRATEGY);
  if (ret != Z_OK) {
    TRPC_FMT_ERROR("deflateInit2 error, ret:{}", ret);
    return false;
  }

  NoncontiguousBufferBuilder builder;
  NoncontiguousBufferOutputStream out_stream(&builder);

  int flush = Z_NO_FLUSH;
  auto itr = in.begin();
  do {
    if (itr != in.end()) {
      strm.next_in = reinterpret_cast<Bytef*>(itr->data());
      strm.avail_in = itr->size();
      ++itr;
    } else {
      flush = Z_FINISH;
      strm.next_in = Z_NULL;
      strm.avail_in = 0;
    }
    // Runs deflate(...) until input drained and output still is not full.
    do {
      void* data = nullptr;
      int size = 0;
      if (!out_stream.Next(&data, &size)) {
        TRPC_FMT_ERROR("NoncontiguousBufferOutputStream::Next failed.");
        (void)deflateEnd(&strm);
        return false;
      }
      strm.avail_out = size;
      strm.next_out = reinterpret_cast<Bytef*>(data);
      ret = deflate(&strm, flush);
      // Z_STREAM_ERROR is unreached.
      TRPC_ASSERT(ret != Z_STREAM_ERROR);
      out_stream.BackUp(strm.avail_out);
    } while (strm.avail_out == 0);
    TRPC_ASSERT(strm.avail_in == 0);
  } while (flush != Z_FINISH);
  TRPC_ASSERT(ret == Z_STREAM_END);

  (void)deflateEnd(&strm);
  out_stream.Flush();
  out = builder.DestructiveGet();
  return true;
}

// Reference: http://www.zlib.net/zlib_how.html
bool Decompress(CompressType type, const NoncontiguousBuffer& in, NoncontiguousBuffer& out) {
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  int ret = inflateInit2(&strm, GetWindowBits(type));
  if (ret != Z_OK) {
    TRPC_FMT_ERROR("inflateInit2 error, ret:{}", ret);
    return false;
  }

  NoncontiguousBufferBuilder builder;
  NoncontiguousBufferOutputStream out_stream(&builder);

  auto itr = in.begin();
  // Runs inflate(...) until input drained and output still is not full.
  do {
    if (!(itr != in.end())) {
      break;
    }
    strm.next_in = reinterpret_cast<Bytef*>(itr->data());
    strm.avail_in = itr->size();
    ++itr;
    if (strm.avail_in == 0) {
      continue;
    }
    TRPC_ASSERT(strm.next_in);

    do {
      void* data = nullptr;
      int size = 0;
      if (!out_stream.Next(&data, &size)) {
        (void)deflateEnd(&strm);
        return false;
      }
      strm.avail_out = size;
      strm.next_out = reinterpret_cast<Bytef*>(data);
      ret = inflate(&strm, Z_NO_FLUSH);
      switch (ret) {
        // Z_STREAM_ERROR means empty input.
        case Z_STREAM_ERROR:
        case Z_NEED_DICT:
          ret = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          (void)inflateEnd(&strm);
          TRPC_FMT_ERROR("Decompress inflate error, ret:{}", ret);
          return false;
      }
      out_stream.BackUp(strm.avail_out);
    } while (strm.avail_out == 0);
  } while (ret != Z_STREAM_END);

  (void)inflateEnd(&strm);
  if (ret != Z_STREAM_END) {
    TRPC_FMT_ERROR("Decompress error, ret:{}", ret);
    return false;
  }
  out_stream.Flush();
  out = builder.DestructiveGet();
  return true;
}

}  // namespace trpc::compressor::zlib
