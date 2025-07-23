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

#include "trpc/compressor/gzip/gzip_compressor.h"

#include "gtest/gtest.h"

namespace trpc::compressor::testing {

TEST(GzipCompressor, Type) {
  GzipCompressor gzip_compressor;
  ASSERT_EQ(gzip_compressor.Type(), kGzip);
}

TEST(GzipCompressor, CompressEmptyStr) {
  for (auto&& level : {kBest, kDefault, kFastest}) {
    NoncontiguousBuffer compress_in;
    NoncontiguousBuffer compress_out;
    GzipCompressor gzip_compressor;
    // Compress empty string.
    ASSERT_TRUE(gzip_compressor.Compress(compress_in, compress_out, level));
    ASSERT_TRUE(!compress_out.Empty());

    // Uncompress empty string.
    NoncontiguousBuffer decompress_out;
    ASSERT_TRUE(gzip_compressor.Decompress(compress_out, decompress_out));
    ASSERT_TRUE(decompress_out.Empty());
  }
}

TEST(GzipCompressor, CompressStr) {
  for (auto&& level : {kBest, kDefault, kFastest}) {
    NoncontiguousBufferBuilder builder;
    builder.Append("hello world");
    NoncontiguousBuffer compress_in = builder.DestructiveGet();
    NoncontiguousBuffer compress_out;
    GzipCompressor gzip_compressor;

    ASSERT_TRUE(gzip_compressor.Compress(compress_in, compress_out, level));
    ASSERT_TRUE(!compress_out.Empty());

    NoncontiguousBuffer decompress_out;
    ASSERT_TRUE(gzip_compressor.Decompress(compress_out, decompress_out));

    ASSERT_EQ(FlattenSlow(decompress_out), FlattenSlow(compress_in));
  }
}

}  // namespace trpc::compressor::testing
