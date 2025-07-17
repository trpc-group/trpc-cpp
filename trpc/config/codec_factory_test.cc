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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/config/testing/codec_plugin_testing.h"

namespace trpc::testing {

class TestCodecPluginTest : public ::testing::Test {
 protected:
  void SetUp() override {
    codec_plugin_ = MakeRefCounted<config::TestCodecPlugin>();
    ASSERT_TRUE(codec_plugin_ != nullptr);
  }

  config::CodecPtr codec_plugin_;
};

TEST_F(TestCodecPluginTest, BasicInformationCheck) {
  EXPECT_EQ(codec_plugin_->Type(), PluginType::kConfigCodec);
  EXPECT_EQ(codec_plugin_->Name(), "TestCodecPlugin");
}

TEST_F(TestCodecPluginTest, DecodeMethod) {
  std::string dummy_data = "dummy data for testing";
  std::unordered_map<std::string, std::any> out;
  bool decode_result = codec_plugin_->Decode(dummy_data, out);
  // Check if the decode function returns true as expected
  EXPECT_TRUE(decode_result);
}

}  // namespace trpc::testing
