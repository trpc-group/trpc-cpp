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

#include "trpc/tvar/common/tvar_group.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "gtest/gtest.h"

#include "trpc/tvar/common/op_util.h"

namespace {

using trpc::tvar::TrpcVarGroup;
using trpc::tvar::detail::ToJsonValue;

class MyClass {
 public:
  explicit MyClass(int64_t v) : value_(v) {}

  explicit MyClass(std::string_view rel_path, int64_t v) : MyClass(v) {
    handle_ = LinkToParent(rel_path);
  }

  MyClass(TrpcVarGroup* parent, std::string_view rel_path, int64_t v) : MyClass(v) {
    handle_ = LinkToParent(rel_path, parent);
  }

  std::string ToString() const { return std::to_string(value_); }

  int64_t GetSeries() { return value_; }

  std::string GetAbsPath() const { return IsExposed() ? handle_->abs_path : std::string(); }

  bool IsExposed() const { return handle_.has_value(); }

  /// @note Make it public for easy test.
  int64_t value_{0};

 private:
  std::optional<TrpcVarGroup::Handle> LinkToParent(std::string_view rel_path,
                                                   TrpcVarGroup* parent) {
    return TrpcVarGroup::LinkToParent(rel_path, parent, [this] { return ToJsonValue(*this); });
  }

  std::optional<TrpcVarGroup::Handle> LinkToParent(std::string_view rel_path) {
    return LinkToParent(rel_path, TrpcVarGroup::FindOrCreate("/"));
  }

 private:
  // Must be last member to avoid race condition.
  std::optional<TrpcVarGroup::Handle> handle_;
};

}  // namespace

namespace trpc::testing {

/// @brief Test for regular usage.
TEST(TvarGroup, Common) {
  MyClass v1("v1", 5);
  ASSERT_EQ(v1.GetAbsPath(), "/v1");

  MyClass v2(TrpcVarGroup::FindOrCreate("/x/y/z"), "v2", 6);
  ASSERT_EQ(v2.GetAbsPath(), "/x/y/z/v2");

  {
    auto opt = TrpcVarGroup::TryGet("/");
    ASSERT_TRUE(opt);
    auto&& jsv = *opt;
    ASSERT_EQ("5", jsv["v1"].asString());
    v1.value_ = 6;
    jsv = *TrpcVarGroup::TryGet("/");
    ASSERT_EQ("6", jsv["v1"].asString());
    v1.value_ = 5;
  }

  {
    auto opt = TrpcVarGroup::TryGet("/");
    ASSERT_TRUE(opt);
    auto&& jsv = *opt;
    Json::StreamWriterBuilder json_builder;
    json_builder["indentation"] = "";
    std::cout << Json::writeString(json_builder, jsv) << std::endl;
    ASSERT_EQ("6", jsv["x"]["y"]["z"]["v2"].asString());
  }
}

/// @brief Test for tvar destructor.
TEST(TvarGroup, DynamicRemoval) {
  {
    MyClass v0("v0", 0);
    ASSERT_EQ(v0.GetAbsPath(), "/v0");
    ASSERT_TRUE(TrpcVarGroup::TryGet("/v0"));
    ASSERT_EQ("0", TrpcVarGroup::TryGet("/v0")->asString());
  }

  ASSERT_FALSE(TrpcVarGroup::TryGet("/v0"));

  {
    MyClass v0("v0", 0);
    ASSERT_EQ(v0.GetAbsPath(), "/v0");
    ASSERT_TRUE(TrpcVarGroup::TryGet("/v0"));
    ASSERT_EQ("0", TrpcVarGroup::TryGet("/v0")->asString());
  }

  ASSERT_FALSE(TrpcVarGroup::TryGet("/v0"));
}

/// @brief Test for slash in path.
TEST(TvarGroup, SlashInPath) {
  MyClass v0(TrpcVarGroup::FindOrCreate("/path/to/var"), R"(\/\/\/\/abc)", 10);
  ASSERT_EQ("10", (*TrpcVarGroup::TryGet("/"))["path"]["to"]["var"]["////abc"].asString());
}

/// @brief Test for setting abort on path.
TEST(SamePathNotAbort, SetWhetherToAbortOnSamePath) {
  trpc::tvar::SetWhetherToAbortOnSamePath(false);
  ASSERT_FALSE(trpc::tvar::AbortOnSamePath());

  trpc::tvar::SetWhetherToAbortOnSamePath(true);
  ASSERT_TRUE(trpc::tvar::AbortOnSamePath());
}

/// @brief Test for FindOrCreate concerned about node and leave.
TEST(SamePathNotAbort, FindOrCreate) {
  trpc::tvar::SetWhetherToAbortOnSamePath(false);

  MyClass v2(TrpcVarGroup::FindOrCreate("/x/y/z"), "v2", 6);
  ASSERT_TRUE(TrpcVarGroup::FindOrCreate("/x/y/z"));
  ASSERT_FALSE(TrpcVarGroup::FindOrCreate("/x/y/z/v2"));
}

/// @brief Test for same path.
TEST(SamePathNotAbort, Common) {
  trpc::tvar::SetWhetherToAbortOnSamePath(false);

  auto v1 = std::make_unique<MyClass>("v1", 5);
  ASSERT_EQ(v1->GetAbsPath(), "/v1");
  ASSERT_TRUE(v1->IsExposed());

  auto v2 = std::make_unique<MyClass>(TrpcVarGroup::FindOrCreate("/x/y/z"), "v2", 6);
  ASSERT_EQ(v2->GetAbsPath(), "/x/y/z/v2");
  ASSERT_TRUE(v2->IsExposed());

  auto repetitive_v1 = std::make_unique<MyClass>("v1", 6);
  ASSERT_FALSE(repetitive_v1->IsExposed());
  ASSERT_TRUE(repetitive_v1->GetAbsPath().empty());

  auto repetitive_v2 = std::make_unique<MyClass>(TrpcVarGroup::FindOrCreate("/x/y/z"), "v2", 7);
  ASSERT_FALSE(repetitive_v2->IsExposed());
  ASSERT_TRUE(repetitive_v2->GetAbsPath().empty());

  v1.reset();
  v2.reset();

  v1 = std::make_unique<MyClass>("v1", 5);
  ASSERT_EQ(v1->GetAbsPath(), "/v1");
  ASSERT_TRUE(v1->IsExposed());

  v2 = std::make_unique<MyClass>(TrpcVarGroup::FindOrCreate("/x/y/z"), "v2", 6);
  ASSERT_EQ(v2->GetAbsPath(), "/x/y/z/v2");
  ASSERT_TRUE(v2->IsExposed());
}

}  // namespace trpc::testing
