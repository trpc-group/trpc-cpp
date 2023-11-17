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
// 1. lz4
// Copyright (C) 2011-2020, Yann Collet
// lz4 is licensed under the BSD 2-Clause and GPL v2.
//
//

#include "trpc/compressor/lz4/lz4_util.h"

#include "trpc/compressor/common/util.h"
#include "trpc/util/buffer/contiguous_buffer.h"
#include "trpc/util/log/logging.h"

namespace trpc::compressor::lz4 {

// The following source codes are from lz4.
// Copied and modified from
// https://github.com/lz4/lz4/blob/dev/examples/frameCompress.c.

constexpr int kInChunkSize = 16 * 1024;  // buffer chunk size

namespace {

// Calculate block size based on frame info
size_t GetBlockSize(const LZ4F_frameInfo_t* info) {
  switch (info->blockSizeID) {
    case LZ4F_default:
    case LZ4F_max64KB:
      return 1 << 16;
    case LZ4F_max256KB:
      return 1 << 18;
    case LZ4F_max1MB:
      return 1 << 20;
    case LZ4F_max4MB:
      return 1 << 22;
    default:
      TRPC_FMT_ERROR("Impossible with expected frame specification, block size id:{}",
                     static_cast<int>(info->blockSizeID));
      return 0;
  }
  return 0;
}

// Generate lz4 frame preferences according to block size id
LZ4F_preferences_t GetLz4Pref(LZ4F_blockSizeID_t block_size_id) {
  LZ4F_preferences_t prefs = LZ4F_INIT_PREFERENCES;
  prefs.frameInfo.blockSizeID = block_size_id;
  return prefs;
}

bool CompressedToOutputStream(trpc::NoncontiguousBufferOutputStream* out_stream, const void* data,
                              std::size_t compressed_size) {
  if (LZ4F_isError(compressed_size)) {
    TRPC_FMT_ERROR("Failed to compression: error {}", LZ4F_getErrorName(compressed_size));
    return false;
  }
  if (compressed_size <= 0) {
    return true;
  }
  if (!CopyDataToOutputStream(data, compressed_size, out_stream)) {
    TRPC_FMT_ERROR("CopyDataToOutputStream error, compressed_size={}", compressed_size);
    return false;
  }
  return true;
}

bool DecompressSingleBuffer(LZ4F_dctx* ctx, const char* in_data, size_t in_size, void* out, size_t out_capacity,
                            trpc::NoncontiguousBufferOutputStream* out_stream) {
  size_t ret = 1;
  // Decompress:
  // Continue while there is more input to read (in_data != in_data_end)
  // and the frame isn't over (ret != 0)
  char* curr_in_data = const_cast<char*>(in_data);
  const char* in_data_end = in_data + in_size;
  while (curr_in_data < in_data_end && ret != 0) {
    // Any data within dst has been flushed at this stage
    size_t out_size = out_capacity;
    size_t curr_in_size = in_data_end - curr_in_data;
    ret = LZ4F_decompress(ctx, out, &out_size, curr_in_data, &curr_in_size, /* LZ4F_decompressOptions_t */ nullptr);
    if (LZ4F_isError(ret)) {
      TRPC_FMT_ERROR("Decompression error: {}", LZ4F_getErrorName(ret));
      return false;
    }
    // Flush output
    if (out_size != 0) {
      if (!CopyDataToOutputStream(out, out_size, out_stream)) {
        TRPC_FMT_ERROR("CopyDataToOutputStream error, out_size={}", out_size);
        return false;
      }
    }
    // Update input
    curr_in_data += curr_in_size;
  }

  if (curr_in_data < in_data_end) {
    TRPC_FMT_ERROR("Decompress: Trailing data left in file after frame");
    return false;
  }
  return true;
}

}  // namespace

bool DoCompress(LZ4F_compressionContext_t& ctx, const NoncontiguousBuffer& in, NoncontiguousBuffer& out) {
  NoncontiguousBufferBuilder builder;
  NoncontiguousBufferOutputStream out_stream(&builder);
  LZ4F_preferences_t perfs = GetLz4Pref(LZ4F_max256KB);

  // large enough for any input <= kInChunkSize
  const size_t out_capacity = LZ4F_compressBound(kInChunkSize, &perfs);
  trpc::BufferPtr out_buff = MakeRefCounted<trpc::Buffer>(out_capacity);

  // write frame header
  {
    const size_t header_size = LZ4F_compressBegin(ctx, out_buff->GetWritePtr(), out_capacity, &perfs);
    if (!CompressedToOutputStream(&out_stream, out_buff->GetReadPtr(), header_size)) {
      TRPC_FMT_ERROR("CompressedToOutputStream error, header_size={}", header_size);
      return false;
    }
  }
  // compress data to buffer
  for (auto itr = in.begin(); itr != in.end(); ++itr) {
    char* in_data = itr->data();
    size_t in_size = itr->size();
    std::size_t current_pos = 0;
    int left_to_copy = static_cast<int>(in_size);
    while (left_to_copy > 0) {
      int current_size = left_to_copy;
      if (left_to_copy > kInChunkSize) {
        current_size = kInChunkSize;
      }
      size_t compressed_size =
          LZ4F_compressUpdate(ctx, out_buff->GetWritePtr(), out_capacity, in_data + current_pos, current_size, nullptr);
      if (!CompressedToOutputStream(&out_stream, out_buff->GetReadPtr(), compressed_size)) {
        TRPC_FMT_ERROR("CompressedToOutputStream error, compressed_size={}", compressed_size);
        return false;
      }
      left_to_copy = -current_size;
      current_pos += current_size;
    }
  }

  // flush whatever remains within internal buffers
  {
    const size_t compressed_size = LZ4F_compressEnd(ctx, out_buff->GetWritePtr(), out_capacity, nullptr);
    if (!CompressedToOutputStream(&out_stream, out_buff->GetReadPtr(), compressed_size)) {
      TRPC_FMT_ERROR("CompressedToOutputStream error, compressed_size={}", compressed_size);
      return false;
    }
  }
  out_stream.Flush();
  out = builder.DestructiveGet();
  return true;
}

bool DoDecompress(LZ4F_dctx* ctx, const NoncontiguousBuffer& in, NoncontiguousBuffer& out) {
  NoncontiguousBufferBuilder builder;
  NoncontiguousBufferOutputStream out_stream(&builder);
  // Read Frame header
  LZ4F_frameInfo_t info;

  auto itr = in.begin();
  char* in_data = nullptr;
  size_t in_size = 0;
  size_t consumed_size = 0;
  if (itr != in.end()) {
    in_data = itr->data();
    in_size = itr->size();
    ++itr;
  }
  if (in_size >= LZ4F_HEADER_SIZE_MAX) {
    consumed_size = in_size;
    const size_t status = LZ4F_getFrameInfo(ctx, &info, in_data, &consumed_size);
    if (LZ4F_isError(status)) {
      TRPC_FMT_ERROR("LZ4F_getFrameInfo error: {}", LZ4F_getErrorName(status));
      return false;
    }
  } else {
    TRPC_ASSERT(kInChunkSize >= LZ4F_HEADER_SIZE_MAX);
    trpc::BufferPtr header_buff = MakeRefCounted<trpc::Buffer>(kInChunkSize);
    memcpy(header_buff->GetWritePtr(), reinterpret_cast<const char*>(in_data), in_size);
    header_buff->AddWriteLen(in_size);
    size_t total_header_buffer_size = in_size;
    size_t total_in_buffer_size = in_size;
    for (; itr != in.end(); ++itr) {
      if (header_buff->ReadableSize() >= LZ4F_HEADER_SIZE_MAX) {
        break;
      }
      in_data = itr->data();
      in_size = itr->size();
      size_t writeable_size = in_size;
      if (in_size > header_buff->WritableSize()) {
        writeable_size = header_buff->WritableSize();
      }
      memcpy(header_buff->GetWritePtr(), reinterpret_cast<const char*>(in_data), writeable_size);
      header_buff->AddWriteLen(writeable_size);
      total_header_buffer_size += writeable_size;
      total_in_buffer_size += in_size;
    }
    size_t buff_consumed_size = total_header_buffer_size;
    const size_t status = LZ4F_getFrameInfo(ctx, &info, header_buff->GetReadPtr(), &buff_consumed_size);
    if (LZ4F_isError(status)) {
      TRPC_FMT_ERROR("LZ4F_getFrameInfo error: {}", LZ4F_getErrorName(status));
      return false;
    }
    size_t left_size = total_in_buffer_size - buff_consumed_size;
    consumed_size = in_size - left_size;
  }
  size_t const out_capacity = GetBlockSize(&info);
  trpc::BufferPtr out_buff = MakeRefCounted<trpc::Buffer>(out_capacity);
  TRPC_ASSERT(consumed_size <= in_size);
  if (!DecompressSingleBuffer(ctx, in_data + consumed_size, in_size - consumed_size, out_buff->GetWritePtr(),
                              out_capacity, &out_stream)) {
    return false;
  }
  for (; itr != in.end(); ++itr) {
    in_data = itr->data();
    in_size = itr->size();
    if (!DecompressSingleBuffer(ctx, in_data, in_size, out_buff->GetWritePtr(), out_capacity, &out_stream)) {
      return false;
    }
  }
  out_stream.Flush();
  out = builder.DestructiveGet();
  return true;
}
// End of source codes that are from lz4.

}  // namespace trpc::compressor::lz4
