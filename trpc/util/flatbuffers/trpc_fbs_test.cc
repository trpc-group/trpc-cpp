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

#include "trpc/util/flatbuffers/trpc_fbs.h"

#include <iostream>

#include "gtest/gtest.h"

#include "trpc/tools/flatbuffers_tool/testing/greeter_generated.h"

namespace flatbuffers::testing {

void ConstructMessage(trpc::Message<::trpc::test::helloworld::HelloReply>& message, const char* data) {
  trpc::MessageBuilder mb;
  auto name_offset = mb.CreateString(data);
  auto rsp_offset = ::trpc::test::helloworld::CreateHelloReply(mb, name_offset);
  mb.Finish(rsp_offset);
  message = mb.ReleaseMessage<::trpc::test::helloworld::HelloReply>();
}

TEST(Message, TestOk) {
  trpc::Message<::trpc::test::helloworld::HelloReply> m1;
  ConstructMessage(m1, "reply");
  trpc::Message<::trpc::test::helloworld::HelloReply> m2;
  const ::trpc::BufferPtr& slice1 = m1.BorrowSlice();
  const ::trpc::BufferPtr& slice2 = m2.BorrowSlice();
  // test operate =
  ASSERT_NE(slice1.Get(), slice2.Get());
  ASSERT_FALSE(slice1->ReadableSize() == 0);
  ASSERT_TRUE(slice2->ReadableSize() == 0);
  m2 = m1;
  ASSERT_EQ(slice1.Get(), slice2.Get());
  m1 = m2;
  ASSERT_EQ(slice1.Get(), slice2.Get());
  // test move
  auto m3 = std::move(m1);
  ASSERT_TRUE(slice1->ReadableSize() == 0);
  const ::trpc::BufferPtr& slice3 = m3.BorrowSlice();
  ASSERT_FALSE(slice3->ReadableSize() == 0);
  ASSERT_EQ(slice3.Get(), slice2.Get());
  auto&& m4 = trpc::Message<::trpc::test::helloworld::HelloReply>();
  const ::trpc::BufferPtr& slice4 = m4.BorrowSlice();
  ASSERT_TRUE(slice4->ReadableSize() == 0);
  trpc::Message<::trpc::test::helloworld::HelloReply> m5(m3);
  const ::trpc::BufferPtr& slice5 = m5.BorrowSlice();
  ASSERT_EQ(slice3.Get(), slice5.Get());
  //  mutable_data
  const uint8_t* md = m3.mutable_data();
  ASSERT_TRUE(md);
  //  data
  const uint8_t* data = m3.data();
  ASSERT_TRUE(data);
  // size
  ASSERT_EQ(m1.size(), 0);
  ASSERT_EQ(m3.size(), 32);
  // ByteSizeLong
  ASSERT_EQ(m1.ByteSizeLong(), 0);
  ASSERT_EQ(m3.ByteSizeLong(), 32);
  //  Verify
  ASSERT_TRUE(m3.Verify());
  //  Since m1 has already been moved to m3, the validation data should be considered invalid.
  ASSERT_FALSE(m1.Verify());
  //  test GetMutableRoot
  auto* rep1 = m3.GetMutableRoot();
  ASSERT_NE(rep1, nullptr);
  //  GetRoot
  auto* rep2 = m3.GetRoot();
  ASSERT_NE(rep2, nullptr);
  ASSERT_EQ(rep2, rep1);
  // HelloReply
  ASSERT_EQ(rep1->message()->str(), std::string("reply"));
  //  Serialize
  char arr[128];
  bool ret = m3.SerializeToArray(arr, 0);
  ASSERT_FALSE(ret);
  ret = m3.SerializeToArray(arr, m3.ByteSizeLong());
  ASSERT_TRUE(ret);
  //  ParseFromArray
  ret = m3.ParseFromArray(arr, m3.ByteSizeLong());
  ASSERT_TRUE(ret);
  // Check if the detection is enabled.
#define FLATBUFFERS_TRPC_DISABLE_AUTO_VERIFICATION
  ret = m3.ParseFromArray(arr, m3.ByteSizeLong());
  ASSERT_TRUE(ret);
  // Failed deserialization.
  ret = m1.ParseFromArray(arr, m1.ByteSizeLong());
  ASSERT_FALSE(ret);
}

TEST(SliceAllocator, TestOk) {
  auto allocator = trpc::detail::SliceAllocatorMember();
  uint8_t* p = allocator.slice_allocator_.allocate(8);
  ASSERT_TRUE(p);
  auto allocator1 = trpc::detail::SliceAllocatorMember();
  trpc::SliceAllocator s1(std::move(allocator1.slice_allocator_));
  uint8_t* p1 = s1.allocate(8);
  uint8_t* p2 = s1.reallocate_downward(p1, 8, 16, 0, 0);
  ASSERT_FALSE(p1 == p2);
}

TEST(MessageBuilder, TestOk) {
  trpc::MessageBuilder mb;
  auto name_offset = mb.CreateString("MessageBuilder");
  auto rsp_offset = ::trpc::test::helloworld::CreateHelloReply(mb, name_offset);
  mb.Finish(rsp_offset);
  size_t size = 0;
  size_t offset = 0;
  ::trpc::BufferPtr slice;
  uint8_t* buf = mb.ReleaseRaw(size, offset, slice);
  // default size is 1024
  ASSERT_EQ(size, 1024);
  ASSERT_TRUE(offset);
  ASSERT_FALSE(slice->ReadableSize() == 0);
  ASSERT_TRUE(buf);
  // set size 2048
  trpc::MessageBuilder mb1(2048);
  name_offset = mb1.CreateString("");
  rsp_offset = ::trpc::test::helloworld::CreateHelloReply(mb1, name_offset);
  mb1.Finish(rsp_offset);
  buf = mb1.ReleaseRaw(size, offset, slice);
  ASSERT_EQ(size, 2048);
  // MessageBuilder
  auto allocator = trpc::detail::SliceAllocatorMember();
  FlatBufferBuilder fb(1024, &allocator.slice_allocator_, false);
  trpc::MessageBuilder m2(std::move(fb), nullptr);
  name_offset = m2.CreateString("");
  rsp_offset = ::trpc::test::helloworld::CreateHelloReply(m2, name_offset);
  m2.Finish(rsp_offset);
  buf = m2.ReleaseRaw(size, offset, slice);
  ASSERT_EQ(size, 1024);

  auto allocator1 = trpc::detail::SliceAllocatorMember();
  FlatBufferBuilder fb1(1024, &allocator1.slice_allocator_, false);
  name_offset = fb1.CreateString("");
  rsp_offset = ::trpc::test::helloworld::CreateHelloReply(fb1, name_offset);
  fb1.Finish(rsp_offset);
  trpc::MessageBuilder m3(std::move(fb1), nullptr);
  buf = m3.ReleaseRaw(size, offset, slice);
  ASSERT_EQ(size, 1024);
  ASSERT_TRUE(buf);
}

}  // namespace flatbuffers::testing
