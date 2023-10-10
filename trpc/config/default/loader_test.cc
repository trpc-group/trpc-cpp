#include "trpc/config/default/loader.h"

#include "gtest/gtest.h"
#include "trpc/config/codec_factory.h"
#include "trpc/config/default/default_config.h"
#include "trpc/config/provider_factory.h"
#include "trpc/config/testing/codec_plugin_testing.h"
#include "trpc/config/testing/provider_plugin_testing.h"

namespace trpc::testing {

class TestLoader : public ::testing::Test {
 protected:
  void SetUp() override {
    default_config_ = MakeRefCounted<config::DefaultConfig>();
    ASSERT_TRUE(default_config_ != nullptr);

    codec_plugin_ = MakeRefCounted<config::TestCodecPlugin>();
    config::CodecFactory::GetInstance()->Register(codec_plugin_);

    provider_plugin_ = MakeRefCounted<config::TestProviderPlugin>();
    config::ProviderFactory::GetInstance()->Register(provider_plugin_);
  }

  config::DefaultConfigPtr default_config_;
  config::ProviderPtr provider_plugin_;
  config::CodecPtr codec_plugin_;
};

TEST_F(TestLoader, CodecOption) {
  auto option1 = config::WithCodec("TestCodecPlugin");
  ASSERT_NO_THROW(option1.Apply(default_config_));
}

TEST_F(TestLoader, ProviderOption) {
  auto option2 = config::WithProvider("TestProviderPlugin");
  ASSERT_NO_THROW(option2.Apply(default_config_));
}

TEST_F(TestLoader, LoadSuccessful) {
  std::string valid_configuration_path = "path/to/your/valid/configuration/file";
  config::DefaultConfigPtr cfg1 = config::detail::Load(
      valid_configuration_path, {config::WithCodec("TestCodecPlugin"), config::WithProvider("TestProviderPlugin")});
  ASSERT_TRUE(cfg1 != nullptr);
  ASSERT_EQ(cfg1->GetCodec().Get(), codec_plugin_.Get());
  ASSERT_EQ(cfg1->GetProvider().Get(), provider_plugin_.Get());

  // Only the registered codec is available
  config::DefaultConfigPtr cfg2 =
      config::detail::Load(valid_configuration_path, {config::WithProvider("TestProviderPlugin")});
  ASSERT_TRUE(cfg2 != nullptr);
  ASSERT_TRUE(cfg2->GetCodec().Get() == nullptr);
  ASSERT_EQ(cfg2->GetProvider().Get(), provider_plugin_.Get());
}

TEST_F(TestLoader, LoadUnsuccessful) {
  std::string invalid_configuration_path = "path/to/your/invalid/configuration/file";

  EXPECT_DEATH(config::DefaultConfigPtr cfg = config::detail::Load(
                   invalid_configuration_path,
                   {config::WithCodec("UnknownCodecPlugin"), config::WithProvider("UnknownProviderPlugin")}),
               "assertion failed: codec != nullptr && \"Codec not found!\"");

  // codec is correct, but provider is not registered
  EXPECT_DEATH(config::DefaultConfigPtr cfg = config::detail::Load(
                   invalid_configuration_path,
                   {config::WithCodec("TestCodecPlugin"), config::WithProvider("UnknownProviderPlugin")}),
               "assertion failed: provider != nullptr && \"Provider not found!\"");

  // codec is correct, but provider is not registered
  EXPECT_DEATH(config::DefaultConfigPtr cfg = config::detail::Load(
                   invalid_configuration_path,
                   {config::WithCodec("UnknownCodecPlugin"), config::WithProvider("TestProviderPlugin")}),
               "assertion failed: codec != nullptr && \"Codec not found!\"");
}

}  // namespace trpc::testing
