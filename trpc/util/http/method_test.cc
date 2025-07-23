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

#include "trpc/util/http/method.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(HttpMethodType, MethodNameToMethodType) {
  struct testing_args_t {
    std::string method_name;
    trpc::http::MethodType expect;
  };

  std::vector<testing_args_t> testings{
      {"GET", trpc::http::MethodType::GET},     {"HEAD", trpc::http::MethodType::HEAD},
      {"PUT", trpc::http::MethodType::PUT},     {"DELETE", trpc::http::MethodType::DELETE},
      {"POST", trpc::http::MethodType::POST},   {"OPTIONS", trpc::http::MethodType::OPTIONS},
      {"PATCH", trpc::http::MethodType::PATCH}, {"DEFAULT", trpc::http::MethodType::UNKNOWN},
  };

  for (const auto& t : testings) {
    ASSERT_EQ(t.expect, trpc::http::MethodNameToMethodType(t.method_name));
    ASSERT_EQ(t.expect, trpc::http::StringToType(t.method_name));
  }
}

TEST(HttpMethodType, MethodName) {
  struct testing_args_t {
    trpc::http::MethodType method_type;
    std::string expect;
  };

  std::vector<testing_args_t> testings{
      {trpc::http::MethodType::GET, "GET"},     {trpc::http::MethodType::HEAD, "HEAD"},
      {trpc::http::MethodType::PUT, "PUT"},     {trpc::http::MethodType::DELETE, "DELETE"},
      {trpc::http::MethodType::POST, "POST"},   {trpc::http::MethodType::OPTIONS, "OPTIONS"},
      {trpc::http::MethodType::PATCH, "PATCH"}, {trpc::http::MethodType::UNKNOWN, ""},
  };

  for (const auto& t : testings) {
    ASSERT_EQ(t.expect, trpc::http::MethodName(t.method_type));
    ASSERT_EQ(t.expect, trpc::http::TypeName(t.method_type));
  }
}

}  // namespace trpc::testing
