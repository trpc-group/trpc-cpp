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

#include <memory>
#include <string>
#include <vector>

#include "trpc/runtime/iomodel/reactor/reactor.h"
#include "trpc/runtime/threadmodel/common/worker_thread.h"

namespace trpc::runtime {

/// @brief Get current worker thread pointer
WorkerThread* GetCurrentThread();

//////////////////////////////////////////////////////////////////////
/// @brief The following interfaces only use in merge threadmodel

/// @brief Get thread num in merge threadmodel by threadmodel instance
int GetMergeThreadNum(const std::string& instance = "default_instance");

/// @brief Get worker thread pointer in merge threadmodel by index and threadmodel instance
WorkerThread* GetMergeThread(uint32_t index, const std::string& instance = "default_instance");

/// @brief Get reactor pointer in merge threadmodel by index and threadmodel instance
Reactor* GetReactor(uint32_t index, const std::string& instance = "default_instance");

/// @brief Get reactor pointer in current thread
Reactor* GetThreadLocalReactor();

//////////////////////////////////////////////////////////////////////
/// @brief The following interfaces only use in separate threadmodel

/// @brief Get io thread num in separate threadmodel by threadmodel instance
int GetIOThreadNum(const std::string& instance = "default_instance");

/// @brief Get io worker thread pointer in separate threadmodel by index and threadmodel instance
WorkerThread* GetIOThread(uint32_t index, const std::string& instance = "default_instance");

/// @brief Get handle thread num in separate threadmodel by threadmodel instance
int GetHandleThreadNum(const std::string& instance = "default_instance");

/// @brief Get handle worker thread pointer in separate threadmodel by index and threadmodel instance
WorkerThread* GetHandleThread(uint32_t index, const std::string& instance = "default_instance");

}  // namespace trpc::runtime
