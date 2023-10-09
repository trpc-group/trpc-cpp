//
//
// Copyright (C) 2014 gRPC authors
// gRPC is licensed under the Apache 2.0 License.
// The source codes in this file based on
// https://github.com/grpc/grpc/blob/v1.39.1/test/core/util/subprocess.h.
// https://github.com/grpc/grpc/blob/v1.39.1/test/cpp/util/subprocess.cc.
// https://github.com/grpc/grpc/blob/v1.39.1/test/core/util/subprocess_posix.cc.
// This source file may have been modified by THL A29 Limited, and licensed under the Apache 2.0 License.
//
//

#include "test/end2end/common/subprocess.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>

namespace trpc::testing {

struct GprSubProcess {
  int pid;
  bool joined{false};
};

GprSubProcess* GprSubProcessCreate(int argc, const char** argv);
void GprSubProcessDestroy(GprSubProcess* p);
int GprSubProcessJoin(GprSubProcess* p);
void GprSubProcessInterrupt(GprSubProcess* p);

static GprSubProcess* MakeProcess(const std::vector<std::string>& args) {
  std::vector<const char*> vargs;
  for (auto it = args.begin(); it != args.end(); ++it) {
    vargs.push_back(it->c_str());
  }
  return GprSubProcessCreate(vargs.size(), &vargs[0]);
}

static GprSubProcess* MakeFnProcess(Function<void(void)>&& fn) {
  GprSubProcess* r{nullptr};
  int pid = 0;

  pid = fork();
  if (pid == -1) {
    return nullptr;
  } else if (pid == 0) {
    fn();
    return nullptr;
  } else {
    r = static_cast<GprSubProcess*>(calloc(sizeof(GprSubProcess), 1));
    r->pid = pid;
    return r;
  }
}

SubProcess::SubProcess(Function<void(void)>&& fn) : subprocess_(MakeFnProcess(std::move(fn))) {}

SubProcess::SubProcess(const std::vector<std::string>& args) : subprocess_(MakeProcess(args)) {}

SubProcess::~SubProcess() { GprSubProcessDestroy(subprocess_); }

int SubProcess::Join() { return GprSubProcessJoin(subprocess_); }

void SubProcess::Interrupt() { GprSubProcessInterrupt(subprocess_); }

GprSubProcess* GprSubProcessCreate(int argc, const char** argv) {
  GprSubProcess* r;
  int pid;
  char** exec_args;

  pid = fork();
  if (pid == -1) {
    return nullptr;
  } else if (pid == 0) {
    exec_args = static_cast<char**>(malloc((static_cast<size_t>(argc) + 1) * sizeof(char*)));
    memcpy(exec_args, argv, static_cast<size_t>(argc) * sizeof(char*));
    exec_args[argc] = nullptr;
    execv(exec_args[0], exec_args);
    /* if we reach here, an error has occurred */
    printf("execv '%s' failed: %s", exec_args[0], strerror(errno));
    _exit(1);
    return nullptr;
  } else {
    r = static_cast<GprSubProcess*>(calloc(sizeof(GprSubProcess), 1));
    r->pid = pid;
    return r;
  }
}

void GprSubProcessDestroy(GprSubProcess* p) {
  if (p) {
    if (!p->joined) {
      kill(p->pid, SIGUSR2);
      GprSubProcessJoin(p);
    }
    free(p);
  }
}

int GprSubProcessJoin(GprSubProcess* p) {
  int status;
retry:
  if (waitpid(p->pid, &status, 0) == -1) {
    if (errno == EINTR) {
      goto retry;
    }
    printf("waitpid failed for pid %d: %s", p->pid, strerror(errno));
    return -1;
  }
  p->joined = true;
  return status;
}

void GprSubProcessInterrupt(GprSubProcess* p) {
  if (!p->joined) {
    kill(p->pid, SIGUSR2);
  }
}

}  // namespace trpc::testing
