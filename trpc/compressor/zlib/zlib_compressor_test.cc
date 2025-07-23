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

#include "trpc/compressor/zlib/zlib_compressor.h"

#include <string>

#include "gtest/gtest.h"
#include "zlib.h"

#include "trpc/compressor/testing/compressor_testing.h"

namespace trpc::compressor::testing {

TEST(ZlibCompressor, Type) {
  ZlibCompressor zlib_compressor;
  ASSERT_EQ(zlib_compressor.Type(), kZlib);
}

TEST(ZlibCompressor, CompressEmptyStr) {
  for (auto&& level : {kBest, kDefault, kFastest}) {
    NoncontiguousBuffer compress_in;
    NoncontiguousBuffer compress_out;
    ZlibCompressor zlib_compressor;

    ASSERT_TRUE(zlib_compressor.Compress(compress_in, compress_out, level));
    ASSERT_TRUE(!compress_out.Empty());

    NoncontiguousBuffer decompress_out;
    ASSERT_TRUE(zlib_compressor.Decompress(compress_out, decompress_out));
    ASSERT_TRUE(decompress_out.Empty());

    auto decompress_in_str = FlattenSlow(compress_out);
    auto decompress_in_len = decompress_in_str.size();
    auto decompress_out_len = compress_in.ByteSize();
    std::string decompress_out_str(decompress_out_len, '\0');
    ASSERT_EQ(uncompress2(reinterpret_cast<Bytef*>(&decompress_out_str[0]), &decompress_out_len,
                          reinterpret_cast<Bytef*>(&decompress_in_str[0]), &decompress_in_len),
              Z_OK);
    ASSERT_EQ(decompress_in_len, compress_out.ByteSize());
    ASSERT_EQ(decompress_out_len, 0);
  }
}

TEST(ZlibCompressor, CompressStr) {
  NoncontiguousBufferBuilder builder;
  builder.Append(GenRandomStr(10 * 1024 * 1024));
  NoncontiguousBuffer compress_in = builder.DestructiveGet();

  for (auto&& level : {kBest, kDefault, kFastest}) {
    NoncontiguousBuffer compress_out;
    ZlibCompressor zlib_compressor;
    ASSERT_TRUE(zlib_compressor.Compress(compress_in, compress_out, level));
    ASSERT_TRUE(!compress_out.Empty());

    NoncontiguousBuffer decompress_out;
    ASSERT_TRUE(zlib_compressor.Decompress(compress_out, decompress_out));

    ASSERT_EQ(FlattenSlow(decompress_out), FlattenSlow(compress_in));

    auto decompress_in_str = FlattenSlow(compress_out);
    auto decompress_in_len = decompress_in_str.size();
    auto decompress_out_len = compress_in.ByteSize();
    std::string decompress_out_str(decompress_out_len, '\0');
    ASSERT_EQ(uncompress2(reinterpret_cast<Bytef*>(&decompress_out_str[0]), &decompress_out_len,
                          reinterpret_cast<Bytef*>(&decompress_in_str[0]), &decompress_in_len),
              Z_OK);
    ASSERT_EQ(decompress_in_len, compress_out.ByteSize());
    ASSERT_EQ(decompress_out_len, compress_in.ByteSize());
    ASSERT_EQ(decompress_out_str, FlattenSlow(compress_in));
  }
}

}  // namespace trpc::compressor::testing
