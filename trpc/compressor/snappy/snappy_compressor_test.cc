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

#include "trpc/compressor/snappy/snappy_compressor.h"

#include <string>

#include "gtest/gtest.h"
#include "snappy.h"

#include "trpc/compressor/testing/compressor_testing.h"

namespace trpc::compressor::testing {

TEST(GzipCompressor, Type) {
  SnappyCompressor compressor;
  // Two testing cases:
  // * Empty string.
  // * Random string (10 MB).
  for (const auto& in : {std::string(), GenRandomStr(10 * 1024 * 1024)}) {
    {
      auto in_data = trpc::CreateBufferSlow(in);
      trpc::NoncontiguousBuffer out_data;
      compressor.Compress(in_data, out_data);

      trpc::NoncontiguousBuffer in_copy;
      compressor.Decompress(out_data, in_copy);
      EXPECT_EQ(trpc::FlattenSlow(in_copy), in);
    }
    {
      auto in_data = trpc::CreateBufferSlow(in);
      trpc::NoncontiguousBuffer out_data;
      compressor.Compress(in_data, out_data);

      std::string out_copy = trpc::FlattenSlow(out_data);
      std::string copy_in;
      EXPECT_TRUE(snappy::Uncompress(out_copy.data(), out_copy.size(), &copy_in));
      EXPECT_EQ(copy_in, in);
    }
    {
      std::string out;
      EXPECT_TRUE(snappy::Compress(in.data(), in.size(), &out) > 0);

      auto out_copy = trpc::CreateBufferSlow(out);
      trpc::NoncontiguousBuffer in_copy;
      compressor.Decompress(out_copy, in_copy);
      EXPECT_EQ(trpc::FlattenSlow(in_copy), in);
    }
  }
}

}  // namespace trpc::compressor::testing
