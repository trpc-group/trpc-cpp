//
//
// Copyright (C) 2014 gRPC authors
// gRPC is licensed under the Apache 2.0 License.
// The source codes in this file based on
// https://github.com/grpc/grpc/blob/v1.39.1/test/cpp/util/subprocess.h.
// This source file may have been modified by Tencent, and licensed under the Apache 2.0 License.
//
//

#pragma once

#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

#include "trpc/util/function.h"

namespace trpc::testing {

struct GprSubProcess;

/// @brief Used to fork a server process for testing. Main process will be as client, which can use gtest to
/// interact server and summary the testing results.
class SubProcess {
 public:
  /// @brief Pass a function and let it be executed as a process.
  /// @param fn function to be executed as a process.
  explicit SubProcess(Function<void(void)>&& fn);

  /// @brief Pass command to start a process.
  /// @param args command to start a process.
  explicit SubProcess(const std::vector<std::string>& args);

  ~SubProcess();

  /// @brief Wait for this process to exit.
  /// @return 0 - process exit successfully, -1 - process exit with error.
  int Join();

  /// @brief Trigger this process to exit.
  void Interrupt();

 private:
  SubProcess(const SubProcess& other);
  SubProcess& operator=(const SubProcess& other);

  GprSubProcess* const subprocess_;
};

}  // namespace trpc::testing
