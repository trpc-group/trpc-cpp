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

namespace trpc {

enum class ClientTransportState {
  kUnknown = 0,          // has not been initialized, next is kInitialized
  kInitialized = 1,      // has been initialized, next is kStopped
  kStopped = 2,          // has been stopped, next is kDestroyed
  kDestroyed = 3,        // has been destroyed, finish
};

}  // namespace trpc
