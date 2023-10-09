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

#include <functional>

#include "trpc/common/status.h"

namespace trpc::coroutine {

using CoroFunction = std::function<Status()>;

class TrpcCoroutineTask {
 public:
  explicit TrpcCoroutineTask(CoroFunction&& f) : func_(std::move(f)) {}

  Status Process() {
    status_ = func_();
    return status_;
  }

  const Status& GetStatus() const { return status_; }

 private:
  CoroFunction func_;
  Status status_;
};

}  // namespace trpc::coroutine
