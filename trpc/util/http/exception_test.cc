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

#include "trpc/util/http/exception.h"

#include <memory>
#include <vector>

#include "gtest/gtest.h"

namespace trpc::testing {

TEST(Exception, what) {
  struct testing_args_t {
    std::shared_ptr<trpc::http::BaseException> e{nullptr};
    std::string what;
    std::string msg;
    int status;
  };

  std::vector<testing_args_t> testings{
      {std::make_shared<trpc::http::NotFoundException>(), "Not Found", "Not Found", 404},
      //      {trpc::http::GatewayTimeoutException{}, "Gateway Timeout", "Gateway Timeout", 504},
      //      {trpc::http::JsonException(trpc::http::RequestTimeout{"Request Timeout"}), "Request Timeout", "Request
      //      Timeout",
      //       504},
  };

  for (const auto& t : testings) {
    ASSERT_EQ(t.what, t.e->what());
    ASSERT_EQ(t.msg, t.e->str());
    ASSERT_EQ(t.status, t.e->Status());
  }

  trpc::http::JsonException e{trpc::http::RequestTimeout{"Request Timeout"}};
  ASSERT_EQ(R"({"message":"Request Timeout","code":504})", e.ToJson());
}

}  // namespace trpc::testing
