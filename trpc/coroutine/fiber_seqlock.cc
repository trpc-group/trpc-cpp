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

#include "trpc/coroutine/fiber_seqlock.h"

namespace trpc {

unsigned FiberSeqLock::ReadSeqBegin() { return ReadSeqCountBegin(&sl_.seqcount); }

unsigned FiberSeqLock::ReadSeqRetry(unsigned retry_num) {
  return ReadSeqCountRetry(&sl_.seqcount, retry_num);
}

void FiberSeqLock::WriteSeqLock() {
  sl_.lock.lock();
  WriteSeqCountBegin(&sl_.seqcount);
}

void FiberSeqLock::WriteSeqUnlock() {
  WriteSeqCountEnd(&sl_.seqcount);
  sl_.lock.unlock();
}

}  // namespace trpc
