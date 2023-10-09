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

#include "test/end2end/common/test_signaller.h"

#include <sys/mman.h>

#include <iostream>

namespace trpc::testing {

TestSignaller::TestSignaller() {
  server_sem_ =
      reinterpret_cast<sem_t*>(mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0));
  client_sem_ =
      reinterpret_cast<sem_t*>(mmap(0, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0));
  if (reinterpret_cast<void*>(server_sem_) == MAP_FAILED || reinterpret_cast<void*>(client_sem_) == MAP_FAILED) {
    std::cerr << "mmap create faield" << std::endl;
    exit(EXIT_FAILURE);
  }
  sem_init(server_sem_, 1, 0);
  sem_init(client_sem_, 1, 0);
}

TestSignaller::~TestSignaller() {
  sem_destroy(server_sem_);
  sem_destroy(client_sem_);
  munmap(reinterpret_cast<void*>(server_sem_), sizeof(sem_t));
  munmap(reinterpret_cast<void*>(client_sem_), sizeof(sem_t));
}

// reference: https://stackoverflow.com/questions/37010836/how-to-correctly-use-sem-timedwait
void WaitWithTimeout(sem_t* sem_var, int timeout) {
  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
    std::cerr << "Error use clock_gettime" << std::endl;
    exit(-1);
  }
  ts.tv_sec += timeout;
  int s;
  while ((s = sem_timedwait(sem_var, &ts)) == -1 && errno == EINTR) {
    // Restart if interrupted by handler
    continue;
  }
  if (s == -1 && errno == ETIMEDOUT) {
    std::cerr << "sem_timedwait() timed out" << std::endl;
    exit(-1);
  }
}

void TestSignaller::ClientWaitToContinue() { WaitWithTimeout(client_sem_, wait_tiemout_); }

void TestSignaller::SignalClientToContinue() { sem_post(client_sem_); }

void TestSignaller::ServerWaitToContinue() { WaitWithTimeout(server_sem_, wait_tiemout_); }

void TestSignaller::SignalServerToContinue() { sem_post(server_sem_); }

}  // namespace trpc::testing
