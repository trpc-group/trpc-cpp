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

#include "trpc/util/buffer/contiguous_buffer.h"

#include <memory>
#include <utility>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(ContiguousBuffer, Construct) {
  char* mem = new char[1024];
  ContiguousBuffer buf_mem(mem, 1024);
  ASSERT_EQ(1024, buf_mem.ReadableSize());

  ContiguousBuffer buf1(0);
  ASSERT_EQ(buf1.GetWritePtr(), nullptr);
  ASSERT_EQ(buf1.GetReadPtr(), nullptr);
  ASSERT_EQ(buf1.WritableSize(), 0);
  ASSERT_EQ(buf1.ReadableSize(), 0);

  buf1 = std::move(buf_mem);
  ASSERT_EQ(buf_mem.GetWritePtr(), nullptr);
  ASSERT_EQ(buf_mem.GetReadPtr(), nullptr);
  ASSERT_EQ(buf_mem.WritableSize(), 0);
  ASSERT_EQ(buf_mem.ReadableSize(), 0);

  ASSERT_EQ(buf1.GetWritePtr(), (mem + 1024));
  ASSERT_EQ(buf1.GetReadPtr(), mem);
  ASSERT_EQ(buf1.WritableSize(), 0);

  trpc::BufferPtr buf_ptr = trpc::MakeRefCounted<ContiguousBuffer>(100);
  ASSERT_EQ(0, buf_ptr->ReadableSize());
  ASSERT_EQ(100, buf_ptr->WritableSize());

  mem = new char[1024];
  mem[0] = 0x12;
  auto copy_ptr = trpc::MakeRefCounted<ContiguousBuffer>(mem, 1024);
  ASSERT_EQ(copy_ptr->GetWritePtr(), mem + 1024);
  ASSERT_EQ(copy_ptr->GetReadPtr(), mem);
  ASSERT_EQ(copy_ptr->WritableSize(), 0);
}

TEST(ContiguousBuffer, ReadAndWrite) {
  ContiguousBuffer buf(1024);

  uint64_t val = 0x1000100010001000;

  memcpy(buf.GetWritePtr(), &val, 8);

  buf.AddWriteLen(8);

  uint64_t tmp = 0;

  memcpy(&tmp, buf.GetReadPtr(), buf.ReadableSize());

  ASSERT_EQ(tmp, val);

  buf.AddReadLen(8);

  val = 0x100000;

  buf.Resize(16);

  ASSERT_EQ(buf.WritableSize(), 16);

  memcpy(buf.GetWritePtr(), &val, 8);

  buf.AddWriteLen(8);

  memcpy(&tmp, buf.GetReadPtr(), buf.ReadableSize());

  ASSERT_EQ(tmp, 0x100000);

  char* mem = new char[8];

  memset(mem, 0x50, 8);

  ContiguousBuffer buf_mem(mem, 8);

  ASSERT_EQ(buf_mem.ReadableSize(), 8);

  memcpy(&tmp, buf_mem.GetReadPtr(), 8);

  ASSERT_EQ(0x5050505050505050, tmp);

  buf_mem.Resize(1024);

  ASSERT_TRUE(buf_mem.WritableSize() == 1024);
}
}  // namespace trpc::testing
