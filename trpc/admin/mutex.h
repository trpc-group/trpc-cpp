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

#include <map>
#include <memory>
#include <string>

namespace trpc::admin {

constexpr int kStackSize{26};
constexpr size_t kMutexMapSize{1024};

struct MutexInfo {
  bool valid;
  int64_t duration_ns;
  pthread_mutex_t* mutex;
};

class SampledContention {
 public:
  SampledContention() : nframes_(-1) {}
  virtual ~SampledContention();

  void DumpAndDestroy(size_t round);
  void Destroy();

  size_t HashCode() const;

  // #Elements in stack.
  int nframes_;
  size_t count_;
  int64_t duration_ns_;
  // Backtrace.
  void* stack_[kStackSize];
};

class ContentionProfiler {
 public:
  explicit ContentionProfiler(const char* name);
  ~ContentionProfiler() {}

  static void FlushTodisk(const std::string& fname,
                          const std::map<std::string, std::shared_ptr<SampledContention>>& tmp_map);

  static bool CheckSysCallInit();

  std::string filename_;
};

int TrpcPthreadMutexLockImpl(pthread_mutex_t* mutex);
int TrpcPthreadMutexUnlockImpl(pthread_mutex_t* mutex);

bool ContentionProfilerStart(const char* fname);
void ContentionProfilerStop();

uint32_t Rotl32(uint32_t x, int8_t r);
uint32_t Fmix32(uint32_t h);
uint64_t Fmix64(uint64_t k);
void MurmurHash3X86And32(const void* key, int len, uint32_t seed, void* out);

int64_t GetSyNaseconds();
uint64_t HashMutexPtr(const pthread_mutex_t* m);

}  // namespace trpc::admin
