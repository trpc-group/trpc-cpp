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

#include <semaphore.h>

namespace trpc::testing {

/// @brief Use semaphores to synchronize the behavior of the process-level server and client.
/// @note For example, the client needs to wait for the server to start before sending data,
///       and the client needs the server to execute a connection disconnection operation when processing data.
/// @note The constructor must be called in the parent process (because inter-process communication is maintained
/// through shared memory).
class TestSignaller {
 public:
  TestSignaller();

  ~TestSignaller();

  /// @brief The client waits to execute.
  void ClientWaitToContinue();

  /// @brief The server can trigger the client to execute.
  void SignalClientToContinue();

  /// @brief The server waits to execute.
  void ServerWaitToContinue();

  /// @brief The client can trigger the server to execute.
  void SignalServerToContinue();

 private:
  sem_t* server_sem_;
  sem_t* client_sem_;
  // client or server will wait for a maximum of 5 seconds. If beyond, it is considered an error and exit directly.
  int wait_tiemout_ = 5;
};

}  // namespace trpc::testing
