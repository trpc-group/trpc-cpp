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

#include "trpc/util/domain_util.h"

#include <set>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace trpc::util::testing {

TEST(DomainUtilTest, GetAddrTypeTest) {
  EXPECT_EQ(trpc::util::GetAddrType("0.1.1.2"), trpc::util::AddrType::ADDR_NX);
  EXPECT_EQ(trpc::util::GetAddrType("127.0.0.1"), trpc::util::AddrType::ADDR_IP);
  EXPECT_EQ(trpc::util::GetAddrType("::1"), trpc::util::AddrType::ADDR_IP);
  EXPECT_EQ(trpc::util::GetAddrType("::"), trpc::util::AddrType::ADDR_IP);
  EXPECT_EQ(trpc::util::GetAddrType(":::"), trpc::util::AddrType::ADDR_NX);
  EXPECT_EQ(trpc::util::GetAddrType("1:2:3:4:5:6:7:8"), trpc::util::AddrType::ADDR_IP);
  EXPECT_EQ(trpc::util::GetAddrType("1:2:3:4:5:6:7"), trpc::util::AddrType::ADDR_NX);
  EXPECT_EQ(trpc::util::GetAddrType("1:2:3:4:5:6:7:8:9"), trpc::util::AddrType::ADDR_NX);
  EXPECT_EQ(trpc::util::GetAddrType("5e:0:0:0:0:0:5668:eeee"), trpc::util::AddrType::ADDR_IP);
  EXPECT_EQ(trpc::util::GetAddrType("5e:0:0:023:0:0:5668:eeee"), trpc::util::AddrType::ADDR_IP);
  EXPECT_EQ(trpc::util::GetAddrType("5e::5668:eeee"), trpc::util::AddrType::ADDR_IP);
  EXPECT_EQ(trpc::util::GetAddrType("::1:8:8888:0:0:8"), trpc::util::AddrType::ADDR_IP);
  EXPECT_EQ(trpc::util::GetAddrType("1::"), trpc::util::AddrType::ADDR_IP);
  EXPECT_EQ(trpc::util::GetAddrType("::1:2:2:2"), trpc::util::AddrType::ADDR_IP);
  EXPECT_EQ(trpc::util::GetAddrType("test.trpc.oa.com"), trpc::util::AddrType::ADDR_DOMAIN);
  EXPECT_EQ(trpc::util::GetAddrType("www.baidu.com"), trpc::util::AddrType::ADDR_DOMAIN);
  EXPECT_EQ(trpc::util::GetAddrType("www.baidu.com."), trpc::util::AddrType::ADDR_DOMAIN);
  EXPECT_EQ(trpc::util::GetAddrType("www.baidu.com.."), trpc::util::AddrType::ADDR_NX);
  EXPECT_EQ(trpc::util::GetAddrType("www.-baidu.com"), trpc::util::AddrType::ADDR_NX);
  EXPECT_EQ(trpc::util::GetAddrType("-www.baidu.com"), trpc::util::AddrType::ADDR_NX);
  EXPECT_EQ(trpc::util::GetAddrType("www. baidu.com"), trpc::util::AddrType::ADDR_NX);
  EXPECT_EQ(trpc::util::GetAddrType("www-12.test.55"), trpc::util::AddrType::ADDR_NX);
  EXPECT_EQ(trpc::util::GetAddrType("localhost"), trpc::util::AddrType::ADDR_DOMAIN);
  EXPECT_EQ(trpc::util::GetAddrType("www_12. "), trpc::util::AddrType::ADDR_NX);
  EXPECT_EQ(trpc::util::GetAddrType("10.3.ww_.kk"), trpc::util::AddrType::ADDR_NX);
}

TEST(DomainUtilTest, LocalhostTest) {
  std::vector<std::string> addrs;

  std::string domain = "localhost";
  trpc::util::GetAddrFromDomain(domain, addrs);
  EXPECT_TRUE(addrs.size() > 0);
  for (const auto& ref : addrs) {
    EXPECT_TRUE(ref == "127.0.0.1" || ref == "::1");
  }
}

TEST(DomainUtilTest, Ipv4NormalTest) {
  std::vector<std::string> addrs;

  std::string domain = "127.0.0.1";
  trpc::util::GetAddrFromDomain(domain, addrs);
  EXPECT_EQ(addrs.size(), 1);
  EXPECT_TRUE(addrs[0] == "127.0.0.1");
}

TEST(DomainUtilTest, Ipv4FailTest) {
  std::vector<std::string> addrs;

  std::string domain = "www.notexistnotexist.com";
  trpc::util::GetAddrFromDomain(domain, addrs);
  EXPECT_EQ(addrs.size(), 0);
}

TEST(DomainUtilTest, Ipv6LocalhostTest) {
  std::vector<std::string> addrs;

  std::string domain = "::1";
  trpc::util::GetAddrFromDomain(domain, addrs);
  if (addrs.size() > 0) {
    EXPECT_TRUE(addrs[0] == "::1");
  }
}

TEST(DomainUtilTest, Ipv6AnyhostTest) {
  std::vector<std::string> addrs;

  std::string domain = "::";
  trpc::util::GetAddrFromDomain(domain, addrs);
  if (addrs.size() > 0) {
    EXPECT_TRUE(addrs[0] == "::");
  }
}

TEST(DomainUtilTest, Ipv6NormalTest) {
  std::vector<std::string> addrs;

  std::string domain = "1:2:3:4:5:6:7:8";
  trpc::util::GetAddrFromDomain(domain, addrs);
  if (addrs.size() > 0) {
    EXPECT_TRUE(addrs[0] == "1:2:3:4:5:6:7:8");
  }
}

TEST(DomainUtilTest, InnerDomainTest) {
  std::vector<std::string> addrs;

  std::string domain = "github.com";
  trpc::util::GetAddrFromDomain(domain, addrs);
  EXPECT_TRUE(addrs.size() > 0);
}

}  // namespace trpc::util::testing
