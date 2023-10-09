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

#include "gtest/gtest.h"

#include "trpc/config/provider/local_file/local_file_provider.h"
#include "trpc/config/trpc_conf.h"

namespace trpc::testing {

struct TrpcConfigProviderTest : public ::testing::Test {
 public:
  void SetUp() {
    ASSERT_TRUE(ConfigHelper::GetInstance()->Init("trpc/config/testing/test.yaml"));
    trpc::config::Init();
  }

  void TearDown() {}
};

TEST_F(TrpcConfigProviderTest, TestLocalYAML) {
  auto config =
      trpc::config::Load("./test_load.yaml", trpc::config::WithCodec("yaml"), trpc::config::WithProvider("file1"));

  // Simple values
  ASSERT_EQ(config->GetInt32("integer", 0), -42);
  ASSERT_EQ(config->GetUint32("unsigned_integer", 0), 123);
  ASSERT_FLOAT_EQ(config->GetFloat("float", 0.0f), 3.14f);
  ASSERT_DOUBLE_EQ(config->GetDouble("double", 0.0), 1.23456789);
  ASSERT_TRUE(config->GetBool("boolean", false));
  ASSERT_EQ(config->GetString("string", ""), "Hello, World!");

  // Map
  ASSERT_EQ(config->GetString("map.key1", ""), "value1");
  ASSERT_EQ(config->GetInt32("map.key2", 0), 123);
  ASSERT_EQ(config->GetString("map.nested_map.nested_key1", ""), "nested_value1");
  ASSERT_FALSE(config->GetBool("map.nested_map.nested_key2", true));

  // List
  ASSERT_EQ(config->GetString("list[0]", ""), "item1");
  ASSERT_EQ(config->GetInt32("list[1]", 0), 456);
  ASSERT_FLOAT_EQ(config->GetFloat("list[2]", 0.0f), 7.89f);
  ASSERT_TRUE(config->GetBool("list[3]", false));

  // Nested list
  ASSERT_EQ(config->GetString("nested_list[0][0]", ""), "nested_item1");
  ASSERT_EQ(config->GetInt32("nested_list[0][1]", 0), 111);
  ASSERT_EQ(config->GetString("nested_list[1][0]", ""), "nested_item2");
  ASSERT_EQ(config->GetInt32("nested_list[1][1]", 0), 222);

  // Mixed map and list
  ASSERT_EQ(config->GetString("mixed.key1[0]", ""), "item1");
  ASSERT_EQ(config->GetInt32("mixed.key1[1]", 0), 333);
  ASSERT_EQ(config->GetString("mixed.key2[0]", ""), "item2");
  ASSERT_EQ(config->GetInt32("mixed.key2[1]", 0), 444);
}

TEST_F(TrpcConfigProviderTest, TestLocalJSON) {
  auto config =
      trpc::config::Load("./test_load.json", trpc::config::WithCodec("json"), trpc::config::WithProvider("file2"));

  // Simple values
  ASSERT_EQ(config->GetInt32("integer", 0), -42);
  ASSERT_EQ(config->GetUint32("unsigned_integer", 0), 123);
  ASSERT_FLOAT_EQ(config->GetFloat("float", 0.0f), 3.14f);
  ASSERT_TRUE(config->GetBool("boolean", false));
  ASSERT_EQ(config->GetString("string", ""), "Hello, World!");

  // Map
  ASSERT_EQ(config->GetString("nested_object.key1", ""), "value1");
  ASSERT_EQ(config->GetInt32("nested_object.key2.sub_key1", 0), 1);
  ASSERT_FALSE(config->GetBool("nested_object.key2.sub_key2", true));

  // List
  for (int i = 0; i < 5; ++i) {
    ASSERT_EQ(config->GetInt32("array[" + std::to_string(i) + "]", 0), i + 1);
  }

  // Nested list
  ASSERT_EQ(config->GetString("nested_list[0][0]", ""), "nested_item1");
  ASSERT_EQ(config->GetInt32("nested_list[0][1]", 0), 111);
  ASSERT_EQ(config->GetString("nested_list[1][0]", ""), "nested_item2");
  ASSERT_EQ(config->GetInt32("nested_list[1][1]", 0), 222);

  // Mixed map and list
  ASSERT_EQ(config->GetString("mixed.key1[0]", ""), "item1");
  ASSERT_EQ(config->GetInt32("mixed.key1[1]", 0), 333);
  ASSERT_EQ(config->GetString("mixed.key2[0]", ""), "item2");
  ASSERT_EQ(config->GetInt32("mixed.key2[1]", 0), 444);

  // Nested list in map
  ASSERT_EQ(config->GetString("nested_object.key3[0].name", ""), "Alice");
  ASSERT_EQ(config->GetInt32("nested_object.key3[0].age", 0), 30);
  ASSERT_EQ(config->GetString("nested_object.key3[1].name", ""), "Bob");
  ASSERT_EQ(config->GetInt32("nested_object.key3[1].age", 0), 25);
}

TEST_F(TrpcConfigProviderTest, TestLocalTOML) {
  auto config =
      trpc::config::Load("./test_load.toml", trpc::config::WithCodec("toml"), trpc::config::WithProvider("file3"));
  ASSERT_EQ(config->GetString("title", ""), "TOML TEST");
  ASSERT_EQ(config->GetString("owner.name", ""), "John Doe");
  ASSERT_EQ(config->GetString("database.server", ""), "192.168.1.1");
  ASSERT_EQ(config->GetInt64("database.connection_max", 0), 5000);
  ASSERT_TRUE(config->GetBool("database.enabled", false));
  ASSERT_EQ(config->GetString("servers.alpha.ip", ""), "10.0.0.1");
  ASSERT_EQ(config->GetString("servers.alpha.dc", ""), "eqdc10");
  ASSERT_EQ(config->GetString("servers.beta.ip", ""), "10.0.0.2");
  ASSERT_EQ(config->GetString("servers.beta.dc", ""), "eqdc10");
}

}  // namespace trpc::testing
