//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

namespace trpc {

enum class RuntimeState {
  kUnknown = 0,    // Not yet initialized, cannot be used. Next phase is kStarted
  kStarted = 1,    // Successfully started, can be used normally. Next phase is kStopped
  kStopped = 2,    // Successfully stopped. Next phase is kDestroyed
  kDestroyed = 3,  // Successfully destroyed, entire lifecycle ended
};

}