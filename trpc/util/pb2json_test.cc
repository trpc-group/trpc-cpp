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

#include "trpc/util/pb2json.h"

#include <string>

#include "gtest/gtest.h"

#include "trpc/util/testing/testjson.pb.h"

namespace trpc::testing {

TEST(Pb2JsonTest, Pb2JsonTest) {
  trpc::test::TestMessage m;
  m.set_msg("test");

  std::string result("");
  bool ret = trpc::Pb2Json::PbToJson(m, &result);

  EXPECT_EQ(true, ret);
  EXPECT_EQ("{\"msg\":\"test\"}", result);

  trpc::test::TestMessage m1;
  ret = trpc::Pb2Json::JsonToPb(result, &m1);

  EXPECT_EQ(true, ret);
  EXPECT_EQ(m1.msg(), m.msg());

  std::string_view str(result.c_str(), result.size());
  trpc::test::TestMessage m2;
  ret = trpc::Pb2Json::JsonToPb(str, &m2);

  EXPECT_EQ(true, ret);
  EXPECT_EQ(m2.msg(), m.msg());

  trpc::test::TestMessage m3;
  ret = trpc::Pb2Json::PbToJson(m3, &result);

  EXPECT_EQ(true, ret);
  EXPECT_EQ("{\"msg\":\"\"}", result);
}

}  // namespace trpc::testing
