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

#include "trpc/client/service_proxy_option_setter.h"

#include <any>
#include <deque>
#include <functional>
#include <list>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/common/config/default_value.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"
#include "trpc/runtime/iomodel/reactor/testing/mock_connection_testing.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

namespace trpc {
namespace detail {

TEST(SetOutputByValidInput, with_string) {
  std::string input = "test";
  std::string output;
  auto user_data = GetValidInput<std::string>(input, "");
  SetOutputByValidInput<std::string>(user_data, output);

  EXPECT_EQ(output, "test");

  input.clear();
  user_data = GetValidInput<std::string>(input, "");
  SetOutputByValidInput<std::string>(user_data, output);
  EXPECT_EQ(output, "test");

  input = "default";
  output = "test";
  std::string default_val = "default";
  user_data = GetValidInput<std::string>(input, default_val);
  SetOutputByValidInput<std::string>(user_data, output);
  ASSERT_EQ(output, "test");

  default_val = "default_test";
  user_data = GetValidInput<std::string>(input, default_val);
  SetOutputByValidInput<std::string>(user_data, output);
  ASSERT_EQ(output, "default");
}

TEST(SetOutputByValidInput, with_uint32_t) {
  uint32_t input = 1000;
  uint32_t output;
  auto user_data = GetValidInput<uint32_t>(input, 0);
  SetOutputByValidInput<uint32_t>(user_data, output);
  EXPECT_EQ(output, 1000);

  input = 0;
  user_data = GetValidInput<uint32_t>(input, 0);
  SetOutputByValidInput<uint32_t>(user_data, output);
  EXPECT_EQ(output, 1000);
}

TEST(SetOutputByValidInput, with_bool) {
  bool input = true;
  bool output;
  auto user_data = GetValidInput<bool>(input, false);
  SetOutputByValidInput<bool>(user_data, output);
  EXPECT_EQ(output, true);

  input = false;
  user_data = GetValidInput<bool>(input, false);
  SetOutputByValidInput<bool>(user_data, output);
  EXPECT_EQ(output, true);

  input = false;
  user_data = GetValidInput<bool>(input, true);
  SetOutputByValidInput<bool>(user_data, output);
  EXPECT_EQ(output, false);
}

int MockCheckProtocol(const ConnectionPtr&, NoncontiguousBuffer&, std::deque<std::any>&) {
  return PacketChecker::PACKET_FULL;
}

TEST(SetOutputByValidInput, with_ProxyCallback) {
  ProxyCallback input;
  input.checker_function =
      std::bind(MockCheckProtocol, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  ProxyCallback output;
  SetOutputByValidInput(input, output);
  auto conn = MakeRefCounted<testing::MockConnection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  auto ret = output.checker_function(conn, in, out);
  EXPECT_EQ(ret, PacketChecker::PACKET_FULL);
}

TEST(SetOutputByValidInput, with_string_vector) {
  std::vector<std::string> input;
  input.push_back("test");
  std::vector<std::string> output;
  SetOutputByValidInput(input, output);
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[0], "test");

  input.clear();
  SetOutputByValidInput(input, output);
  ASSERT_EQ(output.size(), 1);
  EXPECT_EQ(output[0], "test");
}

TEST(SetOutputByValidInput, with_RedisClientConf) {
  RedisClientConf input;
  input.enable = true;
  input.password = "test";
  input.user_name = "user_name";
  input.db = 2;
  RedisClientConf output;
  SetOutputByValidInput(input, output);
  EXPECT_EQ(output.enable, true);
  EXPECT_EQ(output.password, "test");
  EXPECT_EQ(output.user_name, "user_name");
  EXPECT_EQ(output.db, 2);

  input.enable = false;
  input.password = "";
  SetOutputByValidInput(input, output);
  EXPECT_EQ(output.enable, true);
  EXPECT_EQ(output.password, "test");
  EXPECT_EQ(output.user_name, "user_name");
  EXPECT_EQ(output.db, 2);
}

TEST(SetOutputByValidInput, with_ClientSslConfig) {
  ClientSslConfig input;
  input.sni_name = "test";
  ClientSslConfig output;
  SetOutputByValidInput(input, output);
  EXPECT_EQ(output.sni_name, "test");

  input.sni_name = "";
  SetOutputByValidInput(input, output);
  EXPECT_EQ(output.sni_name, "test");
}

TEST(SetOutputByValidInput, with_CustomConfig) {
  std::any input;
  std::any output;
  SetOutputByValidInput(input, output);
  EXPECT_FALSE(output.has_value());

  input = std::string("test");
  SetOutputByValidInput(input, output);
  EXPECT_TRUE(output.has_value());
  EXPECT_EQ(std::any_cast<std::string>(output), "test");
}

TEST(SetDefaultOption, get_default) {
  auto option = std::make_shared<ServiceProxyOption>();
  EXPECT_EQ(option->codec_name, "trpc");
  SetDefaultOption(option);
  EXPECT_EQ(option->codec_name, kDefaultProtocol);
}

TEST(SetSpecifiedOption, set_option) {
  ServiceProxyOption input_option;
  input_option.codec_name = "test";
  auto output_option = std::make_shared<ServiceProxyOption>();
  SetSpecifiedOption(&input_option, output_option);
  EXPECT_EQ(output_option->codec_name, "test");
}

}  // namespace detail

}  // namespace trpc
