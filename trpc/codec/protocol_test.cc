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

#include "trpc/codec/protocol.h"

#include <memory>
#include <utility>

#include "gtest/gtest.h"

#include "trpc/codec/testing/protocol_testing.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc::testing {

class ProtocolTest : public ::testing::Test {
 protected:
  void SetUp() override { protocol_ = std::make_shared<TestProtocol>(); }
  void TearDown() override {}

 protected:
  std::shared_ptr<Protocol> protocol_;
};

TEST_F(ProtocolTest, ZeroCopyDecodeReturnFalse) {
  NoncontiguousBuffer buffer;
  ASSERT_TRUE(protocol_->ZeroCopyDecode(buffer));
}

TEST_F(ProtocolTest, WaitForFullRequestReturnTrue) { ASSERT_TRUE(protocol_->WaitForFullRequest()); }

TEST_F(ProtocolTest, ZeroCopyEncodeReturnFalse) {
  NoncontiguousBuffer buffer;
  ASSERT_TRUE(protocol_->ZeroCopyEncode(buffer));
}

TEST_F(ProtocolTest, GetRequestIdReturnFalse) {
  uint32_t req_id;
  ASSERT_FALSE(protocol_->GetRequestId(req_id));
}

TEST_F(ProtocolTest, SetRequestIdReturnFalse) {
  uint32_t req_id;
  ASSERT_FALSE(protocol_->SetRequestId(req_id));
}

TEST_F(ProtocolTest, GetTimeoutReturnMax) {
  ASSERT_EQ(UINT32_MAX, protocol_->GetTimeout());

  protocol_->SetTimeout(1234);
  ASSERT_EQ(1234, protocol_->GetTimeout());
}

TEST_F(ProtocolTest, GetCallerNameReturnEmpty) { ASSERT_EQ("", protocol_->GetCallerName()); }

TEST_F(ProtocolTest, GetCalleeNameReturnEmpty) { ASSERT_EQ("", protocol_->GetCalleeName()); }

TEST_F(ProtocolTest, GetFuncNameReturnEmpty) { ASSERT_EQ("", protocol_->GetFuncName()); }

TEST_F(ProtocolTest, GetMessageSizeReturnZero) { ASSERT_EQ(0, protocol_->GetMessageSize()); }

TEST_F(ProtocolTest, GetKVInfosReturnEmpty) { ASSERT_EQ(0, protocol_->GetKVInfos().size()); }

TEST_F(ProtocolTest, GetMutableKVInfosReturnNullptr) { ASSERT_NE(nullptr, protocol_->GetMutableKVInfos()); }

TEST_F(ProtocolTest, ByteSizeLongReturnZero) { ASSERT_EQ(0, protocol_->ByteSizeLong()); }

TEST_F(ProtocolTest, GetNonContiguousProtocolBodyReturnEmpty) {
  ASSERT_EQ(0, protocol_->GetNonContiguousProtocolBody().ByteSize());
}

TEST_F(ProtocolTest, GetProtocolAttachmentReturnEmpty) { ASSERT_EQ(0, protocol_->GetProtocolAttachment().ByteSize()); }

}  // namespace trpc::testing
