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

#include "trpc/runtime/iomodel/reactor/common/unix_address.h"

#include "gtest/gtest.h"

namespace trpc {

namespace testing {

const char path[] = "trpc_unix_address_test.socket";

class UnixAddressTest : public ::testing::Test {
 protected:
  void SetUp() override {
    struct sockaddr_un addr_un;
    addr_un.sun_family = AF_UNIX;
    snprintf(addr_un.sun_path, sizeof(addr_un.sun_path), "trpc_unix_address_test.socket");

    default_addr_ = UnixAddress();
    name_addr_ = UnixAddress(path);
    copy_addr_ = UnixAddress(&addr_un);
  }

 protected:
  UnixAddress default_addr_;
  UnixAddress name_addr_;
  UnixAddress copy_addr_;
};

TEST_F(UnixAddressTest, Path) {
  EXPECT_STREQ(name_addr_.Path(), path);
  EXPECT_STREQ(copy_addr_.Path(), path);
}

TEST_F(UnixAddressTest, Sockaddr) {
  EXPECT_TRUE(nullptr != default_addr_.SockAddr());
  EXPECT_TRUE(nullptr != name_addr_.SockAddr());
  EXPECT_TRUE(nullptr != copy_addr_.SockAddr());
}

TEST_F(UnixAddressTest, Socklen) {
  EXPECT_EQ(sizeof(struct sockaddr_un), default_addr_.Socklen());
  EXPECT_EQ(sizeof(struct sockaddr_un), name_addr_.Socklen());
  EXPECT_EQ(sizeof(struct sockaddr_un), copy_addr_.Socklen());
}

}  // namespace testing

}  // namespace trpc
