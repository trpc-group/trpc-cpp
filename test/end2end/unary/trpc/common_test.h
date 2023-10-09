// Copyright (c) 2023, Tencent Inc.
// All rights reserved.

#pragma once

#include "gtest/gtest.h"

#include "test/end2end/common/subprocess.h"
#include "test/end2end/common/util.h"
#include "test/end2end/unary/trpc/trpc_route_server.h"
#include "test/end2end/unary/trpc/trpc_server.h"
#include "test/end2end/unary/trpc/trpc_test.trpc.pb.h"

namespace trpc::testing {

/// @brief Preparing the unary trpc test environment.
class CommonTest : public ::testing::Test {
 public:
  static void SetArgv(int test_argc, char** client_argv, char** server1_argv, char** server2_argv) {
    test_argc_ = test_argc;
    client_argv_ = client_argv;
    server1_argv_ = server1_argv;
    server2_argv_ = server2_argv;
  }

  static void SetUpTestEnv() {
    // Run before the first test case.
    pid_t main_thread_pid_ = getpid();
    server_test_signal_ = std::make_unique<TestSignaller>();
    server_process_ = std::make_unique<SubProcess>([]() {
      TrpcTestServer server(server_test_signal_.get());
      server.Main(test_argc_, server1_argv_);
      server.Wait();
    });

    // Prevent execution of subsequent code when the process exits.
    if (main_thread_pid_ != getpid()) {
      exit(0);
    }

    route_test_signal_ = std::make_unique<TestSignaller>();
    route_process_ = std::make_unique<SubProcess>([]() {
      RouteTestServer server(route_test_signal_.get());
      server.Main(test_argc_, server2_argv_);
      server.Wait();
    });

    // Prevent execution of subsequent code when the process exits.
    if (main_thread_pid_ != getpid()) {
      exit(0);
    }

    // Wait for the server to start.
    server_test_signal_->ClientWaitToContinue();
    route_test_signal_->ClientWaitToContinue();

    InitializeRuntimeEnv(test_argc_, client_argv_);
  }

  static void TearDownTestEnv() {
    // destroy the client environment after the last test case
    DestroyRuntimeEnv();
    // kill the server
    server_process_.reset();
    route_process_.reset();
  }

 protected:
  static std::unique_ptr<TestSignaller> server_test_signal_;
  static std::unique_ptr<TestSignaller> route_test_signal_;
  static std::unique_ptr<SubProcess> server_process_;
  static std::unique_ptr<SubProcess> route_process_;

  static pid_t main_thread_pid_;

 private:
  static int test_argc_;
  static char** client_argv_;
  static char** server1_argv_;
  static char** server2_argv_;
};

std::unique_ptr<TestSignaller> CommonTest::server_test_signal_;
std::unique_ptr<TestSignaller> CommonTest::route_test_signal_;
std::unique_ptr<SubProcess> CommonTest::server_process_;
std::unique_ptr<SubProcess> CommonTest::route_process_;

pid_t CommonTest::main_thread_pid_;

int CommonTest::test_argc_;
char** CommonTest::client_argv_;
char** CommonTest::server1_argv_;
char** CommonTest::server2_argv_;

}  // namespace trpc::testing
