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
// 1. flare
// Copyright (C) 2019 THL A29 Limited, a Tencent company.
// flare is licensed under the BSD 3-Clause License.
//

#include "trpc/util/buffer/noncontiguous_buffer.h"

#include <climits>

#include "gtest/gtest.h"

namespace trpc {

// The following source codes are from flare.
// Copied and modified from
// https://github.com/Tencent/flare/blob/master/flare/base/buffer_test.cc.

TEST(NoncontiguousBuffer, AlphabetLen) { ASSERT_EQ(1 << CHAR_BIT, NoncontiguousBoyerMooreSearcher::kAlphabetLen); }

TEST(NoncontiguousBuffer, Find) {
  NoncontiguousBuffer buffer;
  buffer.Append(CreateBufferSlow("abcdn"));
  buffer.Append(CreateBufferSlow("eed"));
  buffer.Append(CreateBufferSlow("letest"));
  ASSERT_EQ(4, buffer.Find("needle"));
}

TEST(NoncontiguousBuffer, Find2) {
  std::string boundary("--12345--");
  std::string data(
      "--12345\r\n"
      "Content-Disposition: form-data; name=\"somekey\"\r\n"
      "\r\n"
      "Hello; World\r\n"
      "--12345--");
  NoncontiguousBuffer buffer;
  buffer.Append(CreateBufferSlow(data.data(), 21));
  buffer.Append(CreateBufferSlow(data.data() + 21, 54));
  buffer.Append(CreateBufferSlow(data.data() + 75, 2));
  buffer.Append(CreateBufferSlow(data.data() + 77, 4));
  buffer.Append(CreateBufferSlow(data.data() + 81, 1));
  ASSERT_EQ(data, FlattenSlow(buffer));
  ASSERT_EQ(data.find(boundary), buffer.Find(boundary));
}

TEST(NoncontiguousBuffer, FindNotExists) {
  NoncontiguousBuffer buffer;
  buffer.Append(CreateBufferSlow("onexistent"));
  buffer.Append(CreateBufferSlow("nonexisten"));
  buffer.Append(CreateBufferSlow("ntn"));
  ASSERT_EQ(NoncontiguousBuffer::npos, buffer.Find("nonexistent"));
}

TEST(NoncontiguousBuffer, Skip) {
  NoncontiguousBuffer buffer;
  buffer.Append(CreateBufferSlow("asdf"));
  buffer.Append(CreateBufferSlow("asdf"));
  buffer.Append(CreateBufferSlow("asdf"));
  buffer.Append(CreateBufferSlow("asdf"));
  buffer.Append(CreateBufferSlow("asdf"));
  buffer.Append(CreateBufferSlow("asdf"));
  buffer.Append(CreateBufferSlow("asdf"));
  buffer.Append(CreateBufferSlow("asdf"));
  buffer.Skip(32);
  ASSERT_EQ(0, buffer.ByteSize());
}

TEST(NoncontiguousBuffer, Skip2) {
  NoncontiguousBuffer buffer;
  EXPECT_TRUE(buffer.Empty());
  buffer.Skip(0);  // Don't crash.
  EXPECT_TRUE(buffer.Empty());
}

TEST(NoncontiguousBuffer, Size) {
  NoncontiguousBuffer buffer;
  EXPECT_TRUE(buffer.Empty());
  ASSERT_EQ(buffer.size(), 0);
  buffer.Append(CreateBufferSlow("one"));
  ASSERT_EQ(buffer.size(), 1);
  buffer.Append(CreateBufferSlow("two"));
  ASSERT_EQ(buffer.size(), 2);
}

TEST(NoncontiguousBuffer, Copy) {
  std::string data(100000, 'a');

  NoncontiguousBufferBuilder builder;
  builder.Append(data);
  auto buf = builder.DestructiveGet();

  auto buf2 = buf;

  ASSERT_EQ(FlattenSlow(buf), data);
  ASSERT_EQ(FlattenSlow(buf2), data);
}

TEST(NoncontiguousBuffer, CreateBufferBlockSlow) {
  char buff[16] = {'a'};
  object_pool::LwUniquePtr<BufferBlock> block = CreateBufferBlockSlow(buff, 16);
  ASSERT_EQ(block->size(), 16);
}

TEST(NoncontiguousBuffer, FlattenToSlow) {
  NoncontiguousBuffer small_buffer;
  std::string src = "small";
  small_buffer.Append(CreateBufferSlow(src));

  std::string dest;
  dest.resize(src.size());

  FlattenToSlow(small_buffer, dest.data(), dest.size());

  ASSERT_EQ(src, dest);
}

static void BufferBlockReset(BufferBlock& block) {
  RefPtr<memory_pool::MemBlock> mem = MakeBlockRef(memory_pool::Allocate());
  block.Reset(0, 10, mem);
  ASSERT_EQ(block.size(), 10);
  ASSERT_TRUE(block.data() == mem->data);
}

// The main test is for a char pointer type wrapped object.
TEST(BufferBlock, WrapUpPtr) {
  // 1、Initialize the block first.
  BufferBlock block;
  ASSERT_EQ(block.size(), 0);

  // 2、Construct a block of NoncontiguousBuffer and perform the test.
  BufferBlockReset(block);

  // 3、Construct a pointer to a contiguous buffer.
  char* ptr = static_cast<char*>(new char[16]);
  // Since a non-contiguous buffer was assigned earlier, an assert should be triggered here
  // according to the principle of not mixing buffer types.
  ASSERT_DEATH(block.WrapUp(ptr, 16), "");

  // 4、Clean up the block for testing with a contiguous buffer.
  block.Clear();

  // Test the wrapped address ptr. This time there is no mixing, so it should be correct.
  block.WrapUp(ptr, 16);
  ASSERT_EQ(block.size(), 16);
  ASSERT_TRUE(block.data() == ptr);
  // It is not allowed to take ownership again.
  ASSERT_DEATH(block.WrapUp(ptr, 16), "");
  // Fully take ownership and execute Clear to release the memory of `ptr`.
  block.Clear();
}

// The main test is for a ContiguousBuffer type wrapped object.
TEST(BufferBlock, WrapUpContiguousBuffer) {
  // 1、Initialize the block first.
  BufferBlock block;
  ASSERT_EQ(block.size(), 0);

  // 2、Construct a block of NoncontiguousBuffer and perform the test.
  BufferBlockReset(block);

  // 3、Test WrapUp function for BUFFER_INNER type.
  auto contiguous_buffer = MakeRefCounted<ContiguousBuffer>(1000);
  ASSERT_DEATH(block.WrapUp(std::move(contiguous_buffer)), "");
  ASSERT_EQ(contiguous_buffer->WritableSize(), 1000);
  block.Clear();

  // 4、Test wrap address for contiguous_buffer.
  contiguous_buffer->AddWriteLen(8);
  const char* read_ptr = contiguous_buffer->GetReadPtr();
  block.WrapUp(std::move(contiguous_buffer));
  ASSERT_EQ(block.size(), 8);
  ASSERT_TRUE(block.data() == read_ptr);

  // 5、Multiple ownership is not supported.
  char* ptr = new char[16];
  // It is not allowed to take ownership again.
  ASSERT_DEATH(block.WrapUp(ptr, 16), "");
  block.Clear();

  // 6、Test WrapUp function for BUFFER_DELEGATE type.
  contiguous_buffer = MakeRefCounted<ContiguousBuffer>(ptr, 16);
  block.WrapUp(std::move(contiguous_buffer));
  ASSERT_EQ(block.size(), 16);
  ASSERT_TRUE(block.data() == ptr);

  // 7、 Test skip.
  block.Skip(1);
  ASSERT_EQ(block.size(), 16 - 1);
  ASSERT_TRUE(block.data() == (ptr + 1));

  // Clear up block
  block.Clear();
}

// Add a ptr to a NoncontiguousBuffer.
TEST(NoncontiguousBuffer, AppendPtr) {
  // 1、Construct the test data.
  NoncontiguousBufferBuilder nb;
  // Add a data.
  nb.Append('a');
  NoncontiguousBuffer buff = nb.DestructiveGet();
  NoncontiguousBuffer test_buff;

  // 2、Test Wrap. At this point, NoncontiguousBuffer data has been placed in buff.
  std::size_t len = 64;
  char* ptr = static_cast<char*>(new char[len]);
  const char* c_ptr = ptr;
  ASSERT_DEATH(buff.Append(ptr, len), "");
  // Clear buffer
  buff.Clear();

  // 3、Appending again should be successful.
  buff.Append(ptr, len);
  ASSERT_DEATH(buff.Append(c_ptr, len), "");
  ASSERT_EQ(buff.ByteSize(), len);
  ASSERT_EQ(buff.size(), 1);
  ASSERT_TRUE(buff.FirstContiguous().data() == ptr);
  ASSERT_TRUE(buff.FirstContiguous().size() == len);
  // It is not allowed to take ownership again.
  ASSERT_DEATH(buff.Append(ptr, len), "");
  // Mixing is not allowed.
  ASSERT_DEATH(buff.Append(test_buff), "");
  auto block = object_pool::MakeLwUnique<BufferBlock>();
  char* ptr1 = static_cast<char*>(new char[len]);
  block->WrapUp(ptr1, len);
  // Mixing is not allowed.
  ASSERT_DEATH(buff.Append(std::move(block)), "");
  auto contiguous_buffer = MakeRefCounted<ContiguousBuffer>(1000);
  ASSERT_DEATH(buff.Append(std::move(contiguous_buffer)), "");
  ASSERT_EQ(contiguous_buffer->WritableSize(), 1000);

  // 4、Test skip.
  buff.Skip(len / 2);
  ASSERT_TRUE(buff.FirstContiguous().data() == (ptr + len / 2));
  ASSERT_TRUE(buff.FirstContiguous().size() == (len - len / 2));

  // 5、Test cut.
  NoncontiguousBuffer new_buff = buff.Cut(10);
  ASSERT_TRUE(new_buff.FirstContiguous().data() == (ptr + len / 2));
  ASSERT_TRUE(new_buff.FirstContiguous().size() == 10);
  ASSERT_TRUE(buff.FirstContiguous().data() == (ptr + len / 2 + 10));
  ASSERT_TRUE(buff.FirstContiguous().size() == (len - len / 2 - 10));

  // Clean up this block.
  buff.Clear();
}

// Test contiguous buffer object.
TEST(NoncontiguousBuffer, AppendContiguous) {
  // 1、Construct the test data.
  NoncontiguousBufferBuilder nb;
  nb.Append('b');
  NoncontiguousBuffer buff = nb.DestructiveGet();

  // 2、Test append function. The buffer has already been placed in a BufferBlock.
  auto contiguous_buffer = MakeRefCounted<ContiguousBuffer>(1000);
  ASSERT_DEATH(buff.Append(std::move(contiguous_buffer)), "");
  ASSERT_EQ(contiguous_buffer->WritableSize(), 1000);

  // Clean up this block.
  buff.Clear();

  // 3、Test append function for BUFFER_INNER type.
  contiguous_buffer->AddWriteLen(8);
  const char* read_ptr = contiguous_buffer->GetReadPtr();
  buff.Append(std::move(contiguous_buffer));
  ASSERT_EQ(buff.ByteSize(), 8);
  ASSERT_EQ(buff.size(), 1);
  ASSERT_TRUE(buff.FirstContiguous().size() == 8);
  ASSERT_TRUE(buff.FirstContiguous().data() == read_ptr);
  buff.Clear();  // Clean up this block.

  // 4、Test append function for BUFFER_DELEGATE type.
  std::size_t len = 64;
  char* ptr = new char[len];
  const char* c_ptr = ptr;

  contiguous_buffer = MakeRefCounted<ContiguousBuffer>(ptr, len);
  buff.Append(std::move(contiguous_buffer));
  ASSERT_DEATH(buff.Append(c_ptr, len), "");
  ASSERT_EQ(buff.ByteSize(), len);
  ASSERT_EQ(buff.size(), 1);
  ASSERT_TRUE(buff.FirstContiguous().size() == len);
  ASSERT_TRUE(buff.FirstContiguous().data() == ptr);

  // 5、Test multiple append calls.
  // Cannot perform append again.
  ASSERT_DEATH(buff.Append(ptr, len), "");
  // Mixing is not allowed.
  NoncontiguousBuffer test_buff;
  ASSERT_DEATH(buff.Append(test_buff), "");
  auto block = object_pool::MakeLwUnique<BufferBlock>();
  char* ptr1 = new char[len];
  block->WrapUp(ptr1, len);
  // Mixing is not allowed.
  ASSERT_DEATH(buff.Append(std::move(block)), "");

  // 6、Test skip function.
  buff.Skip(len / 2);
  ASSERT_TRUE(buff.FirstContiguous().data() == (ptr + len / 2));
  ASSERT_TRUE(buff.FirstContiguous().size() == (len - len / 2));

  // 7、Test using NoncontiguousBuffer interface on contiguous buffer.
  NoncontiguousBuffer new_buff = buff.Cut(10);
  ASSERT_TRUE(new_buff.FirstContiguous().data() == (ptr + len / 2));
  ASSERT_TRUE(new_buff.FirstContiguous().size() == 10);
  ASSERT_TRUE(buff.FirstContiguous().data() == (ptr + len / 2 + 10));
  ASSERT_TRUE(buff.FirstContiguous().size() == (len - len / 2 - 10));

  buff.Clear();
}

TEST(NoncontiguousBuffer, BufferBlock) {
  BufferBlock b1;
  ASSERT_TRUE(b1.data() == nullptr);

  RefPtr<memory_pool::MemBlock> mem = MakeBlockRef(memory_pool::Allocate());
  auto* p1 = mem->data;
  b1.Reset(0, 10, mem);
  ASSERT_TRUE(b1.data() != nullptr);
  ASSERT_TRUE(b1.data() == p1);
  ASSERT_TRUE(b1.size() == 10);

  BufferBlock b2(std::move(b1));
  ASSERT_TRUE(b1.data() == nullptr);
  ASSERT_TRUE(b1.size() == 0);

  ASSERT_TRUE(b2.data() == p1);
  ASSERT_TRUE(b2.size() == 10);

  b2.Skip(2);
  auto* p2 = b2.data();
  ASSERT_TRUE(p2 == (p1 + 2));
  ASSERT_TRUE(b2.data() == (p1 + 2));
  ASSERT_TRUE(b2.size() == 8);

  b1 = std::move(b2);
  ASSERT_TRUE(b1.data() == (p1 + 2));
  ASSERT_TRUE(b1.size() == 8);
  ASSERT_TRUE(b2.data() == nullptr);
  ASSERT_TRUE(b2.size() == 0);
}

}  // namespace trpc
