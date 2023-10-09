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

#pragma once

#include "test/end2end/stream/trpc/async_stream_server.h"
#include "test/end2end/stream/trpc/sync_stream_server.h"

#include "gtest/gtest.h"

#include "test/end2end/common/subprocess.h"
#include "test/end2end/common/util.h"
#include "test/end2end/stream/trpc/stream.trpc.pb.h"

namespace trpc::testing {

/// @brief Preparing the streaming test environment.
class StreamTest : public ::testing::Test {
 public:
  static void SetArgv(int test_argc, char** client_argv, char** server1_argv, char** server2_argv) {
    test_argc_ = test_argc;
    client_argv_ = client_argv;
    server1_argv_ = server1_argv;
    server2_argv_ = server2_argv;
  }

  static void SetUpTestSuite() {
    // Run before the first test case.
    pid_t main_thread_pid_ = getpid();
    sync_test_signal_ = std::make_unique<TestSignaller>();
    sync_server_process_ = std::make_unique<SubProcess>([]() {
      SyncStreamServer server(sync_test_signal_.get());
      server.Main(test_argc_, server1_argv_);
      server.Wait();
    });

    // Prevent execution of subsequent code when the process exits.
    if (main_thread_pid_ != getpid()) {
      exit(0);
    }

    async_test_signal_ = std::make_unique<TestSignaller>();
    async_server_process_ = std::make_unique<SubProcess>([]() {
      AsyncStreamServer server(async_test_signal_.get());
      server.Main(test_argc_, server2_argv_);
      server.Wait();
    });

    // Prevent execution of subsequent code when the process exits.
    if (main_thread_pid_ != getpid()) {
      exit(0);
    }

    // Wait for the server to start.
    sync_test_signal_->ClientWaitToContinue();
    async_test_signal_->ClientWaitToContinue();

    InitializeRuntimeEnv(test_argc_, client_argv_);
  }

  static void TearDownTestSuite() {
    // Run after the last test case.
    // Destroy the client environment.
    DestroyRuntimeEnv();
    // Kill the server
    sync_server_process_.reset();
    async_server_process_.reset();
  }

 protected:
  static std::unique_ptr<TestSignaller> sync_test_signal_;
  static std::unique_ptr<SubProcess> sync_server_process_;

  static std::unique_ptr<TestSignaller> async_test_signal_;
  static std::unique_ptr<SubProcess> async_server_process_;

  static pid_t main_thread_pid_;

 private:
  static int test_argc_;
  static char** client_argv_;
  static char** server1_argv_;
  static char** server2_argv_;
};

std::unique_ptr<TestSignaller> StreamTest::sync_test_signal_;
std::unique_ptr<SubProcess> StreamTest::sync_server_process_;

std::unique_ptr<TestSignaller> StreamTest::async_test_signal_;
std::unique_ptr<SubProcess> StreamTest::async_server_process_;

pid_t StreamTest::main_thread_pid_;

int StreamTest::test_argc_;
char** StreamTest::client_argv_;
char** StreamTest::server1_argv_;
char** StreamTest::server2_argv_;

}  // namespace trpc::testing
