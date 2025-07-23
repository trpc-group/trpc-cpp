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

#include "trpc/codec/codec_helper.h"

#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"

namespace trpc::testing {

TEST(CodecHelperTest, GetDefaultServerRetCode) {
  ASSERT_TRUE(trpc::codec::Init());
  int result = trpc::codec::GetDefaultServerRetCode(trpc::codec::ServerRetCode::SUCCESS);
  ASSERT_EQ(0, result);
  result = trpc::codec::GetDefaultServerRetCode(trpc::codec::ServerRetCode::TIMEOUT_ERROR);
  ASSERT_EQ(21, result);
  result = trpc::codec::GetDefaultServerRetCode(trpc::codec::ServerRetCode::OVERLOAD_ERROR);
  ASSERT_EQ(22, result);
  result = trpc::codec::GetDefaultServerRetCode(trpc::codec::ServerRetCode::LIMITED_ERROR);
  ASSERT_EQ(23, result);
  result = trpc::codec::GetDefaultServerRetCode(trpc::codec::ServerRetCode::FULL_LINK_TIMEOUT_ERROR);
  ASSERT_EQ(24, result);
  result = trpc::codec::GetDefaultServerRetCode(trpc::codec::ServerRetCode::ENCODE_ERROR);
  ASSERT_EQ(2, result);
  result = trpc::codec::GetDefaultServerRetCode(trpc::codec::ServerRetCode::DECODE_ERROR);
  ASSERT_EQ(1, result);
  result = trpc::codec::GetDefaultServerRetCode(trpc::codec::ServerRetCode::NOT_FUN_ERROR);
  ASSERT_EQ(12, result);
  result = trpc::codec::GetDefaultServerRetCode(trpc::codec::ServerRetCode::AUTH_ERROR);
  ASSERT_EQ(41, result);
  result = trpc::codec::GetDefaultServerRetCode(trpc::codec::ServerRetCode::INVOKE_UNKNOW_ERROR);
  ASSERT_EQ(999, result);
}

TEST(CodecHelperTest, GetDefaultClientRetCode) {
  ASSERT_TRUE(trpc::codec::Init());
  int result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::SUCCESS);
  ASSERT_EQ(0, result);
  result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::INVOKE_TIMEOUT_ERR);
  ASSERT_EQ(101, result);
  result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::FULL_LINK_TIMEOUT_ERROR);
  ASSERT_EQ(102, result);
  result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::CONNECT_ERROR);
  ASSERT_EQ(111, result);
  result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::ENCODE_ERROR);
  ASSERT_EQ(121, result);
  result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::DECODE_ERROR);
  ASSERT_EQ(122, result);
  result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::LIMITED_ERROR);
  ASSERT_EQ(123, result);
  result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::OVERLOAD_ERROR);
  ASSERT_EQ(124, result);
  result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::ROUTER_ERROR);
  ASSERT_EQ(131, result);
  result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::NETWORK_ERROR);
  ASSERT_EQ(141, result);
  result = trpc::codec::GetDefaultClientRetCode(trpc::codec::ClientRetCode::INVOKE_UNKNOW_ERROR);
  ASSERT_EQ(999, result);
}

TEST(CodecHelperTest, OutOfBound) {
  ASSERT_TRUE(trpc::codec::Init());

  int result = trpc::codec::GetDefaultServerRetCode(static_cast<trpc::codec::ServerRetCode>(-1));
  ASSERT_EQ(result, -1);

  result = trpc::codec::GetDefaultClientRetCode(static_cast<trpc::codec::ClientRetCode>(-1));
  ASSERT_EQ(result, -1);
}

TEST(CodecHelperTest, ClientRetToUType) {
  ASSERT_EQ(ToUType(codec::ClientRetCode::SUCCESS), 0);
  ASSERT_EQ(ToUType(codec::ClientRetCode::INVOKE_TIMEOUT_ERR), 1);
  ASSERT_EQ(ToUType(codec::ClientRetCode::FULL_LINK_TIMEOUT_ERROR), 2);
  ASSERT_EQ(ToUType(codec::ClientRetCode::CONNECT_ERROR), 3);
  ASSERT_EQ(ToUType(codec::ClientRetCode::ENCODE_ERROR), 4);
  ASSERT_EQ(ToUType(codec::ClientRetCode::DECODE_ERROR), 5);
  ASSERT_EQ(ToUType(codec::ClientRetCode::LIMITED_ERROR), 6);
  ASSERT_EQ(ToUType(codec::ClientRetCode::OVERLOAD_ERROR), 7);
  ASSERT_EQ(ToUType(codec::ClientRetCode::ROUTER_ERROR), 8);
  ASSERT_EQ(ToUType(codec::ClientRetCode::NETWORK_ERROR), 9);
  ASSERT_EQ(ToUType(codec::ClientRetCode::INVOKE_UNKNOW_ERROR), 10);
}

TEST(CodecHelperTest, ServerRetToUType) {
  ASSERT_EQ(ToUType(codec::ServerRetCode::SUCCESS), 0);
  ASSERT_EQ(ToUType(codec::ServerRetCode::TIMEOUT_ERROR), 1);
  ASSERT_EQ(ToUType(codec::ServerRetCode::OVERLOAD_ERROR), 2);
  ASSERT_EQ(ToUType(codec::ServerRetCode::LIMITED_ERROR), 3);
  ASSERT_EQ(ToUType(codec::ServerRetCode::FULL_LINK_TIMEOUT_ERROR), 4);
  ASSERT_EQ(ToUType(codec::ServerRetCode::ENCODE_ERROR), 5);
  ASSERT_EQ(ToUType(codec::ServerRetCode::DECODE_ERROR), 6);
  ASSERT_EQ(ToUType(codec::ServerRetCode::NOT_FUN_ERROR), 7);
  ASSERT_EQ(ToUType(codec::ServerRetCode::AUTH_ERROR), 8);
  ASSERT_EQ(ToUType(codec::ServerRetCode::INVOKE_UNKNOW_ERROR), 9);
}

}  // namespace trpc::testing
