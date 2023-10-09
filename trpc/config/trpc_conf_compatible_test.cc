#include "trpc/config/trpc_conf_compatible.h"

#include "gtest/gtest.h"
#include "json/json.h"
#include "yaml-cpp/yaml.h"

namespace trpc::testing {

class TrpcConfCompatibleTest : public ::testing::Test {
 protected:
  void SetUp() override {}
};

TEST_F(TrpcConfCompatibleTest, GetString_Success) {
  Json::Value config_json;
  config_json["key1"] = "value1";
  config_json["key2"] = "value2";

  std::string result;
  ASSERT_TRUE(trpc::config::GetString(config_json, "key1", &result));
  ASSERT_EQ(result, "value1");
}

TEST_F(TrpcConfCompatibleTest, GetString_Fail) {
  Json::Value config_json;
  config_json["key1"] = "value1";
  config_json["key2"] = "value2";

  std::string result;
  ASSERT_FALSE(trpc::config::GetString(config_json, "key3", &result));
}

TEST_F(TrpcConfCompatibleTest, GetBool_Success) {
  Json::Value config_json;
  config_json["setting1"] = true;
  config_json["setting2"] = false;

  bool result;
  ASSERT_TRUE(trpc::config::GetBool(config_json, "setting1", &result));
  ASSERT_EQ(result, true);

  ASSERT_TRUE(trpc::config::GetBool(config_json, "setting2", &result));
  ASSERT_EQ(result, false);
}

TEST_F(TrpcConfCompatibleTest, GetBool_Fail) {
  Json::Value config_json;
  config_json["setting1"] = true;
  config_json["setting2"] = false;

  bool result;
  ASSERT_FALSE(trpc::config::GetBool(config_json, "setting3", &result));
}

TEST_F(TrpcConfCompatibleTest, GetInt64_Success) {
  Json::Value config_json;
  config_json["value1"] = 42;
  config_json["value2"] = -42;

  int64_t result;
  ASSERT_TRUE(trpc::config::GetInt64(config_json, "value1", &result));
  ASSERT_EQ(result, 42);

  ASSERT_TRUE(trpc::config::GetInt64(config_json, "value2", &result));
  ASSERT_EQ(result, -42);
}

TEST_F(TrpcConfCompatibleTest, GetInt64_Fail) {
  Json::Value config_json;
  config_json["value1"] = 42;
  config_json["value2"] = -42;

  int64_t result;
  ASSERT_FALSE(trpc::config::GetInt64(config_json, "value3", &result));
}

TEST_F(TrpcConfCompatibleTest, GetUint64_Success) {
  Json::Value config_json;
  config_json["value1"] = 42;
  config_json["value2"] = 84;

  uint64_t result;
  ASSERT_TRUE(trpc::config::GetUint64(config_json, "value1", &result));
  ASSERT_EQ(result, 42u);

  ASSERT_TRUE(trpc::config::GetUint64(config_json, "value2", &result));
  ASSERT_EQ(result, 84u);
}

TEST_F(TrpcConfCompatibleTest, GetUint64_Fail) {
  Json::Value config_json;
  config_json["value1"] = 42;
  config_json["value2"] = 84;

  uint64_t result;
  ASSERT_FALSE(trpc::config::GetUint64(config_json, "value3", &result));
}

TEST_F(TrpcConfCompatibleTest, GetDouble_Success) {
  Json::Value config_json;
  config_json["value1"] = 3.14159;
  config_json["value2"] = -3.14159;

  double result;
  ASSERT_TRUE(trpc::config::GetDouble(config_json, "value1", &result));
  ASSERT_EQ(result, 3.14159);

  ASSERT_TRUE(trpc::config::GetDouble(config_json, "value2", &result));
  ASSERT_EQ(result, -3.14159);
}

TEST_F(TrpcConfCompatibleTest, GetDouble_Fail) {
  Json::Value config_json;
  config_json["value1"] = 3.14159;
  config_json["value2"] = -3.14159;

  double result;
  ASSERT_FALSE(trpc::config::GetDouble(config_json, "value3", &result));
}

TEST_F(TrpcConfCompatibleTest, TransformConfig_JSON_Success) {
  std::string initial_config = "{\"key1\":\"value1\",\"key2\":\"value2\"}";
  Json::Value result;

  ASSERT_TRUE(trpc::config::TransformConfig(initial_config, &result));
  ASSERT_EQ(result["key1"].asString(), "value1");
  ASSERT_EQ(result["key2"].asString(), "value2");
}

TEST_F(TrpcConfCompatibleTest, TransformConfig_YAML_Success) {
  std::string initial_config = "key1: value1\nkey2: value2";
  YAML::Node result;

  ASSERT_TRUE(trpc::config::TransformConfig(initial_config, &result));
  ASSERT_EQ(result["key1"].as<std::string>(), "value1");
  ASSERT_EQ(result["key2"].as<std::string>(), "value2");
}

TEST_F(TrpcConfCompatibleTest, GetYamlValue) {
  YAML::Node node = YAML::Load("key: value\nnested:\n  key2: value2");
  ASSERT_EQ(trpc::config::GetYamlValue(node, "key"), "value");
  ASSERT_EQ(trpc::config::GetYamlValue(node, "nested.key2"), "value2");
}

TEST_F(TrpcConfCompatibleTest, ExtractYamlKeys) {
  YAML::Node node = YAML::Load("key1: value1\nkey2: value2");
  std::vector<std::string> yaml_keys;

  ASSERT_TRUE(trpc::config::ExtractYamlKeys(node, "", yaml_keys));
  ASSERT_EQ(yaml_keys.size(), 2);
  ASSERT_EQ(yaml_keys[0], "key1");
  ASSERT_EQ(yaml_keys[1], "key2");
}

}  // namespace trpc::testing
