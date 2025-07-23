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

#include "trpc/compressor/lz4/lz4_compressor.h"

#include <string>

#include "gtest/gtest.h"

#include "lz4.h"
#include "lz4frame.h"

#include "trpc/compressor/testing/compressor_testing.h"
#include "trpc/util/buffer/buffer.h"

namespace trpc::compressor::testing {

TEST(Lz4FrameCompressor, Type) {
  Lz4FrameCompressor lz4_compressor;
  ASSERT_EQ(lz4_compressor.Type(), kLz4Frame);
}

TEST(Lz4FrameCompressor, CompressStr) {
  Lz4FrameCompressor compressor;

  for (const auto& in : {std::string(), std::string("ab"), GenRandomStr(10 * 1024 * 1024)}) {
    {
      trpc::NoncontiguousBuffer compress_in = trpc::CreateBufferSlow(in);
      trpc::NoncontiguousBuffer compress_out;
      compressor.Compress(compress_in, compress_out);

      trpc::NoncontiguousBuffer decompress_out;
      compressor.Decompress(compress_out, decompress_out);
      EXPECT_EQ(trpc::FlattenSlow(decompress_out), in);
    }

    {
      trpc::NoncontiguousBuffer compress_in = trpc::CreateBufferSlow(in);
      trpc::NoncontiguousBuffer compress_out;
      compressor.Compress(compress_in, compress_out);

      auto decompress_in_str = FlattenSlow(compress_out);
      auto decompress_in_len = decompress_in_str.size();
      auto decompress_out_len = compress_in.ByteSize();
      std::string decompress_out_str(decompress_out_len, '\0');
      ASSERT_TRUE(LZ4_decompress_safe(decompress_in_str.data(), decompress_out_str.data(), decompress_in_len,
                                      decompress_out_len));
      EXPECT_EQ(decompress_in_len, compress_out.ByteSize());
      EXPECT_EQ(decompress_out_len, in.size());
    }

    {
      LZ4F_compressionContext_t ctx;
      size_t ctx_status = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
      EXPECT_FALSE(LZ4F_isError(ctx_status));
      LZ4F_preferences_t prefs = LZ4F_INIT_PREFERENCES;
      prefs.frameInfo.blockSizeID = LZ4F_max256KB;

      int max_output_size = LZ4F_compressBound(in.size(), &prefs);
      std::string out(max_output_size, '\0');
      trpc::BufferPtr out_buff = MakeRefCounted<trpc::Buffer>(max_output_size * 2);  // Stores the result of compression

      size_t count_out = 0;
      size_t header_size = LZ4F_compressBegin(ctx, out.data(), max_output_size, &prefs);
      EXPECT_FALSE(LZ4F_isError(header_size));
      if (header_size > 0) {
        memcpy(out_buff->GetWritePtr(), out.data(), header_size);
      }
      count_out += header_size;

      size_t compressed_size = LZ4F_compressUpdate(ctx, out.data(), max_output_size, in.data(), in.size(), nullptr);
      EXPECT_FALSE(LZ4F_isError(compressed_size));
      if (compressed_size > 0) {
        memcpy(out_buff->GetWritePtr() + count_out, out.data(), compressed_size);
      }
      count_out += compressed_size;

      size_t final_size = LZ4F_compressEnd(ctx, out.data(), max_output_size, nullptr);
      if (final_size > 0) {
        memcpy(out_buff->GetWritePtr() + count_out, out.data(), final_size);
      }
      count_out += final_size;
      LZ4F_freeCompressionContext(ctx);

      trpc::NoncontiguousBuffer decompress_in = trpc::CreateBufferSlow(out_buff->GetReadPtr(), count_out);
      trpc::NoncontiguousBuffer decompress_out;
      compressor.Decompress(decompress_in, decompress_out);
      EXPECT_EQ(trpc::FlattenSlow(decompress_out), in);
    }
  }
}

}  // namespace trpc::compressor::testing
