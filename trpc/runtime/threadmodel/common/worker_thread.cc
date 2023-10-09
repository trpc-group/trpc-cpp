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

#include "trpc/runtime/threadmodel/common/worker_thread.h"

#include "trpc/util/thread/thread_helper.h"

namespace trpc {

void WorkerThread::SetCurrentThreadName(WorkerThread* current) {
  uint32_t thread_index = (static_cast<uint32_t>(current->GroupId()) << 16) + current->Id();

  std::string thread_name("");
  if ((current->Role() & kIo) == kIo) {
      thread_name += "iothread_";
  } else {
      thread_name += "handlethread_";
  }
  thread_name += std::to_string(thread_index);
  thread_name += "_";
  thread_name += current->GetGroupName();

  trpc::SetCurrentThreadName(thread_name);
}

}  // namespace trpc
