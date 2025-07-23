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

#include "zlib.h"

#include "gtest/gtest.h"

namespace trpc::compressor::zlib::testing {

TEST(ZlibUtilTest, ConvertLevel) {
  ASSERT_EQ(ConvertLevel(kBest), 9);
  ASSERT_EQ(ConvertLevel(kFastest), 1);
  ASSERT_EQ(ConvertLevel(kDefault), Z_DEFAULT_COMPRESSION);
}

TEST(ZlibUtilTest, GetWindowBits) {
  ASSERT_EQ(GetWindowBits(kGzip), MAX_WBITS + 16);
  ASSERT_EQ(GetWindowBits(kZlib), MAX_WBITS);
}

}  // namespace trpc::compressor::zlib::testing
