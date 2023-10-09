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

#include <memory>
#include <string>
#include <utility>

#include "rapidjson/document.h"

#include "trpc/util/http/handler.h"

#ifdef TRPC_ENABLE_PROFILER
extern "C" {
// bool __attribute__((weak)) ProfilingIsEnabledForAllThreads();
bool ProfilingIsEnabledForAllThreads();
// int __attribute__((weak)) ProfilerStart(const char* fname);
int ProfilerStart(const char* fname);
// void __attribute__((weak)) ProfilerStop();
void ProfilerStop();

// bool __attribute__((weak)) IsHeapProfilerRunning();
bool IsHeapProfilerRunning();
// int __attribute__((weak)) HeapProfilerStart(const char* fname);
int HeapProfilerStart(const char* fname);
// void __attribute__((weak)) HeapProfilerStop();
void HeapProfilerDump(const char* reason);
void HeapProfilerStop();

char* GetHeapProfile();
}
#endif

namespace trpc {

/// @brief Handler holds the main logic for serving input commands by HTTP.
/// All derived handlers inherit from AdminHandlerBase and implement the CommandHandle method.
class AdminHandlerBase : public http::HandlerBase {
 public:
  ~AdminHandlerBase() override = default;

  /// @brief Handles commands input by user.
  virtual void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                             rapidjson::Document::AllocatorType& alloc) = 0;

  /// @brief Serves HTTP requests (implements the "Handle" interface of HTTP handler).
  trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, http::HttpRequestPtr req,
                      http::HttpResponse* rsp) override;

  /// @brief Gets description of the handler.
  const std::string& Description() const { return description_; }

 protected:
  std::string description_;
};

}  // namespace trpc
