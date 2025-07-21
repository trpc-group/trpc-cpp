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

#pragma once

#include "gtest/gtest.h"

#include "test/end2end/common/subprocess.h"
#include "test/end2end/common/test_signaller.h"
#include "test/end2end/common/util.h"
#include "test/end2end/unary/tvar/tvar.trpc.pb.h"
#include "trpc/client/http/http_service_proxy.h"

extern int test_argc;
extern char** client_argv;
extern char** server_argv;

namespace trpc::testing {

class TestTvar : public ::testing::Test {
 public:
  static void SetUpTestSuite();
  static void TearDownTestSuite();

 public:
  static std::shared_ptr<trpc::http::HttpServiceProxy> http_proxy_;
  static std::shared_ptr<trpc::testing::tvar::TvarServiceProxy> tvar_proxy_;

 private:
  static std::unique_ptr<TestSignaller> test_signal_;
  static std::unique_ptr<SubProcess> server_process_;
};

}  // namespace trpc::testing
