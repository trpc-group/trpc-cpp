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

#include "trpc/util/buffer/buffer.h"

#include <utility>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(BufferTest, ContiguousToNonContiguousTest) {
  NoncontiguousBuffer noncontiguous_buffer;

  BufferPtr contiguous_buffer = MakeRefCounted<Buffer>(1024);
  ASSERT_FALSE(ContiguousToNonContiguous(contiguous_buffer, noncontiguous_buffer));

  contiguous_buffer->AddWriteLen(8);
  ASSERT_TRUE(ContiguousToNonContiguous(contiguous_buffer, noncontiguous_buffer));
  ASSERT_EQ(noncontiguous_buffer.ByteSize(), 8);
  ASSERT_EQ(contiguous_buffer->ReadableSize(), 0);
}

TEST(BufferTest, NonContiguousToContiguousTest) {
  NoncontiguousBuffer noncontiguous_buffer;
  BufferPtr contiguous_buffer;
  ASSERT_FALSE(NonContiguousToContiguous(noncontiguous_buffer, contiguous_buffer));

  noncontiguous_buffer.Append(CreateBufferSlow("asdf"));
  ASSERT_TRUE(NonContiguousToContiguous(noncontiguous_buffer, contiguous_buffer));

  contiguous_buffer = MakeRefCounted<Buffer>(1024);
  ASSERT_TRUE(NonContiguousToContiguous(noncontiguous_buffer, contiguous_buffer));
}

}  // namespace trpc
