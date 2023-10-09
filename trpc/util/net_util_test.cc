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

#include "trpc/util/net_util.h"

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace trpc::testing {

class NetUtilTestValid : public ::testing::TestWithParam<std::string> {};

TEST_P(NetUtilTestValid, test) {
  const std::string &ipstr = GetParam();

  bool ok = false;
  auto ipint = util::StringToIpv4(ipstr, &ok);
  EXPECT_TRUE(ok);

  ok = false;
  auto ipstr2 = util::Ipv4ToString(ipint, &ok);
  EXPECT_TRUE(ok);

  EXPECT_EQ(ipstr, ipstr2);
}

INSTANTIATE_TEST_SUITE_P(test, NetUtilTestValid,
                        ::testing::Values("0.0.0.0", "1.1.1.1", "59.56.54.51", "255.255.255.255"));

class NetUtilTestInvalid : public ::testing::TestWithParam<std::string> {};

TEST_P(NetUtilTestInvalid, test) {
  const std::string &ipstr = GetParam();

  bool ok = true;
  auto ipint = util::StringToIpv4(ipstr, &ok);
  EXPECT_FALSE(ok);
  EXPECT_EQ(ipint, 0);
}

INSTANTIATE_TEST_SUITE_P(test, NetUtilTestInvalid,
                        ::testing::Values("", "-1.1.1.1", "0.0.0", "1.1..1", "1.1.1.1.",
                                          "255.255.255.256", "256.0.0.1"));

class ParseHostPortInvalid : public ::testing::TestWithParam<std::string> {};

TEST_P(ParseHostPortInvalid, Ipv4Host) {
  const std::string &name = GetParam();
  std::string host;
  int port{0};
  bool is_ipv6{false};
  EXPECT_FALSE(util::ParseHostPort(name, host, port, is_ipv6));
  EXPECT_TRUE(host.empty());
  EXPECT_EQ(0, port);
  EXPECT_FALSE(is_ipv6);
}

INSTANTIATE_TEST_SUITE_P(TestIpv4Invalid, ParseHostPortInvalid,
                        ::testing::Values("127.0.0.1:abc", "127.0.0.1 10001",
                                          "127.0.0.1:", "127.0.0.1"));

INSTANTIATE_TEST_SUITE_P(TestIpv6Invalid, ParseHostPortInvalid,
                        ::testing::Values("[::1:10001", "::1]:10001", "[::1]:abc", "[::1] 10001",
                                          "[::1]:", "[::1]", "::1"));

INSTANTIATE_TEST_SUITE_P(TestDomainInvalid, ParseHostPortInvalid,
                        ::testing::Values("www.baidu.com:abc", "www.baidu.com 10001",
                                          "www.baidu.com:", "www.baidu.com"));

TEST(ParseHostPortTest, TestValid) {
  // ipv4:port
  {
    std::string host;
    int port{0};
    bool is_ipv6{false};
    EXPECT_TRUE(util::ParseHostPort("127.0.0.1:10001", host, port, is_ipv6));
    EXPECT_EQ("127.0.0.1", host);
    EXPECT_EQ(10001, port);
    EXPECT_FALSE(is_ipv6);
  }
  // [ipv6]:port
  {
    std::string host;
    int port{0};
    bool is_ipv6{false};
    EXPECT_TRUE(util::ParseHostPort("[::1]:10001", host, port, is_ipv6));
    EXPECT_EQ("::1", host);
    EXPECT_EQ(10001, port);
    EXPECT_TRUE(is_ipv6);
  }
  // domain:port
  {
    std::string host;
    int port{0};
    bool is_ipv6{false};
    EXPECT_TRUE(util::ParseHostPort("www.baidu.com:10001", host, port, is_ipv6));
    EXPECT_EQ("www.baidu.com", host);
    EXPECT_EQ(10001, port);
    EXPECT_FALSE(is_ipv6);
  }
}

TEST(GetIpByEthTest, TestFunction) {
  std::string eth("lo");
  EXPECT_EQ("127.0.0.1", trpc::util::GetIpByEth(eth));

  eth = "lo1";
  EXPECT_EQ("", trpc::util::GetIpByEth(eth));
}

TEST(GenRandomAvailablePortTest, TestFunction) {
  int port = trpc::util::GenRandomAvailablePort();
  EXPECT_TRUE(port >= 10000);
}

}  // namespace trpc::testing
