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

#include <cstdint>

namespace trpc::runtime {

constexpr uint32_t kFinish = 0;
constexpr uint32_t kRequestMsg = 1;
constexpr uint32_t kResponseMsg = 2;
constexpr uint32_t kActiveCloseConnection = 3;
constexpr uint32_t kTimer = 4;
constexpr uint32_t kParallelTask = 5;

}  // namespace trpc::runtime
