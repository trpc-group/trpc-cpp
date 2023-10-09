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

#include "trpc/compressor/trpc_compressor.h"

#include "gtest/gtest.h"

#include "trpc/compressor/compressor_factory.h"
#include "trpc/compressor/testing/compressor_testing.h"
#include "trpc/util/buffer/buffer.h"

namespace trpc::compressor::testing {

TEST(TrpcCompressor, Init) {
  ASSERT_TRUE(Init());
  Destroy();
}

TEST(TrpcCompressor, CompressSuccess) {
  CompressType type{132};
  CompressorFactory::GetInstance()->Clear();
  auto p = MakeRefCounted<MockCompressor>();
  EXPECT_CALL(*p, Type()).WillOnce(::testing::Return(type));
  EXPECT_CALL(*p, DoCompress(::testing::_, ::testing::_, ::testing::_)).WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*p, DoDecompress(::testing::_, ::testing::_)).WillRepeatedly(::testing::Return(true));
  ASSERT_TRUE(CompressorFactory::GetInstance()->Register(p));

  NoncontiguousBuffer compress_in = CreateBufferSlow("hello world");
  ASSERT_TRUE(CompressIfNeeded(type, compress_in));

  NoncontiguousBuffer decompress_in = CreateBufferSlow("hello world");
  ASSERT_TRUE(DecompressIfNeeded(type, decompress_in));

  NoncontiguousBuffer in;
  NoncontiguousBuffer out;
  ASSERT_TRUE(Compress(type, in, out));
  ASSERT_TRUE(Compress(type, in, out));
}

TEST(TrpcCompressor, CompressFail) {
  CompressType type{133};
  auto p = MakeRefCounted<MockCompressor>();
  EXPECT_CALL(*p, Type()).WillOnce(::testing::Return(type));
  EXPECT_CALL(*p, DoCompress(::testing::_, ::testing::_, ::testing::_))
      .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*p, DoDecompress(::testing::_, ::testing::_)).WillRepeatedly(::testing::Return(false));
  CompressorFactory::GetInstance()->Clear();
  ASSERT_TRUE(CompressorFactory::GetInstance()->Register(p));

  NoncontiguousBuffer compress_in = CreateBufferSlow("hello world");
  ASSERT_FALSE(CompressIfNeeded(type, compress_in));
  NoncontiguousBuffer decompress_in = CreateBufferSlow("hello world");
  ASSERT_FALSE(DecompressIfNeeded(type, decompress_in));

  NoncontiguousBuffer in = CreateBufferSlow("hello world");
  NoncontiguousBuffer out;
  ASSERT_FALSE(Compress(type, in, out));
  ASSERT_FALSE(Compress(type, in, out));
}

TEST(TrpcCompressor, CompressorNotSupport) {
  CompressType type{134};
  CompressorFactory::GetInstance()->Clear();

  NoncontiguousBuffer compress_in = CreateBufferSlow("hello world");
  ASSERT_FALSE(CompressIfNeeded(type, compress_in));
  NoncontiguousBuffer decompress_in = CreateBufferSlow("hello world");
  ASSERT_FALSE(DecompressIfNeeded(type, decompress_in));

  NoncontiguousBuffer in = CreateBufferSlow("hello world");
  NoncontiguousBuffer out;
  ASSERT_FALSE(Compress(type, in, out));
  ASSERT_FALSE(Compress(type, in, out));
}

TEST(TrpcCompressor, CompressorTypeNone) {
  NoncontiguousBuffer compress_in = CreateBufferSlow("hello world");
  ASSERT_TRUE(CompressIfNeeded(kNone, compress_in));
  NoncontiguousBuffer decompress_in = CreateBufferSlow("hello world");
  ASSERT_TRUE(DecompressIfNeeded(kNone, decompress_in));

  NoncontiguousBuffer in = CreateBufferSlow("hello world");
  NoncontiguousBuffer out;
  ASSERT_FALSE(Compress(kNone, in, out));
  ASSERT_FALSE(Compress(kMaxType, in, out));
  ASSERT_FALSE(Decompress(kNone, in, out));
  ASSERT_FALSE(Decompress(kMaxType, in, out));
}

TEST(TrpcCompressor, CompressorTypeMaxType) {
  NoncontiguousBuffer compress_in = CreateBufferSlow("hello world");
  ASSERT_FALSE(CompressIfNeeded(kMaxType, compress_in));
  NoncontiguousBuffer decompress_in = CreateBufferSlow("hello world");
  ASSERT_FALSE(DecompressIfNeeded(kMaxType, decompress_in));

  NoncontiguousBuffer in = CreateBufferSlow("hello world");
  NoncontiguousBuffer out;
  ASSERT_FALSE(Compress(kMaxType, in, out));
  ASSERT_FALSE(Decompress(kMaxType, in, out));
}

}  // namespace trpc::compressor::testing
