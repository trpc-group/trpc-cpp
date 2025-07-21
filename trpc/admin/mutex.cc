//
//
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
// The source codes in this file based on
// https://github.com/apache/brpc/blob/0.9.6-rc02/src/bthread/mutex.cpp.
// This source file may have been modified by Tencent, and licensed under the Apache 2.0 License.
//
//

#include "trpc/admin/mutex.h"

#include <dlfcn.h>  // dlsym
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>  // O_RDONLY
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include <cinttypes>
#include <mutex>
#include <sstream>
#include <utility>

#include "trpc/util/log/logging.h"

#define BIG_CONSTANT(x) (x##LLU)

namespace trpc::admin {

extern "C" {
extern void* __attribute__((weak)) _dl_sym(void* handle, const char* symbol, void* caller);
}

///////////////////////////////////////////////////////////////////////////////////
class StatisticContentionProfiler {
 public:
  StatisticContentionProfiler() {}

  virtual ~StatisticContentionProfiler() {}

  static bool CheckContProfObj();

  static uint64_t cp_version_;
  static ContentionProfiler* cp_obj_;
  static std::mutex cp_mutex_;
};

uint64_t StatisticContentionProfiler::cp_version_ = 0;
ContentionProfiler* StatisticContentionProfiler::cp_obj_ = nullptr;
std::mutex StatisticContentionProfiler::cp_mutex_;

bool StatisticContentionProfiler::CheckContProfObj() { return StatisticContentionProfiler::cp_obj_ == nullptr; }

static __thread MutexInfo mutex_map_[kMutexMapSize] = {};
std::map<std::string, std::shared_ptr<SampledContention>> sample_map_;

///////////////////////////////////////////////////////////////////////////////////
uint32_t Rotl32(uint32_t x, int8_t r) { return (x << r) | (x >> (32 - r)); }

uint32_t Fmix32(uint32_t h) {
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

uint64_t Fmix64(uint64_t k) {
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;
  return k;
}

void MurmurHash3X86And32(const void* key, int len, uint32_t seed, void* out) {
  const uint8_t* data = (const uint8_t*)key;
  const int nblocks = len / 4;

  uint32_t h1 = seed;

  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;

  // body
  const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);

  for (int i = -nblocks; i; i++) {
    uint32_t k1 = blocks[i];

    k1 *= c1;
    k1 = Rotl32(k1, 15);
    k1 *= c2;

    h1 ^= k1;
    h1 = Rotl32(h1, 13);
    h1 = h1 * 5 + 0xe6546b64;
  }

  // tail
  const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);

  uint32_t k1 = 0;
  switch (len & 3) {
    case 3:
      k1 ^= tail[2] << 16;
    case 2:
      k1 ^= tail[1] << 8;
    case 1:
      k1 ^= tail[0];
      k1 *= c1;
      k1 = Rotl32(k1, 15);
      k1 *= c2;
      h1 ^= k1;
  }

  // finalization

  h1 ^= len;
  h1 = Fmix32(h1);
  *(reinterpret_cast<uint32_t*>(out)) = h1;
}

int64_t GetSyNaseconds() {
  timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now.tv_sec * 1000000000L + now.tv_nsec;
}

uint64_t HashMutexPtr(const pthread_mutex_t* m) { return Fmix64((uint64_t)m); }

///////////////////////////////////////////////////////////////////////////////////
typedef int (*MutexOp)(pthread_mutex_t*);

struct SystemCallFuncApi {
  MutexOp hook_pthread_mutex_lock;
  MutexOp hook_pthread_mutex_unlock;
};

static SystemCallFuncApi sys_call_list;

static void InitSysMutexLock() {
  if (!sys_call_list.hook_pthread_mutex_lock) {
    sys_call_list.hook_pthread_mutex_lock =
        (MutexOp)_dl_sym(RTLD_NEXT, "pthread_mutex_lock", reinterpret_cast<void*>(InitSysMutexLock));
  }

  if (!sys_call_list.hook_pthread_mutex_unlock) {
    sys_call_list.hook_pthread_mutex_unlock =
        (MutexOp)_dl_sym(RTLD_NEXT, "pthread_mutex_unlock", reinterpret_cast<void*>(InitSysMutexLock));
  }
}

int TrpcPthreadMutexLockImpl(pthread_mutex_t* mutex) {
  InitSysMutexLock();
  if (!StatisticContentionProfiler::cp_obj_) {
    return sys_call_list.hook_pthread_mutex_lock(mutex);
  }

  int rc = pthread_mutex_trylock(mutex);
  if (rc != EBUSY) {
    return rc;
  }

  const int64_t start_ns = GetSyNaseconds();
  rc = sys_call_list.hook_pthread_mutex_lock(mutex);
  if (!rc) {
    size_t idx = HashMutexPtr(mutex) & (kMutexMapSize - 1);
    MutexInfo& minfo = mutex_map_[idx];
    minfo.valid = true;
    minfo.mutex = mutex;
    minfo.duration_ns = GetSyNaseconds() - start_ns;
  }
  return rc;
}

int TrpcPthreadMutexUnlockImpl(pthread_mutex_t* mutex) {
  InitSysMutexLock();
  if (!StatisticContentionProfiler::cp_obj_) {
    return sys_call_list.hook_pthread_mutex_unlock(mutex);
  }

  int64_t unlock_start_ns = GetSyNaseconds();
  const int rc = sys_call_list.hook_pthread_mutex_unlock(mutex);
  const int64_t duration_ns = GetSyNaseconds() - unlock_start_ns;

  size_t idx = HashMutexPtr(mutex) & (kMutexMapSize - 1);
  MutexInfo& minfo = mutex_map_[idx];
  if (minfo.valid) {
    std::shared_ptr<SampledContention> sc_ptr = std::make_shared<SampledContention>();
    sc_ptr->duration_ns_ = minfo.duration_ns + duration_ns;
    sc_ptr->count_ = 1;
    sc_ptr->nframes_ = backtrace(sc_ptr->stack_, kStackSize);

    size_t hash_code = sc_ptr->HashCode();
    std::stringstream tmp_stream;
    tmp_stream << reinterpret_cast<void*>(mutex);
    std::string hask_key = tmp_stream.str() + "-" + std::to_string(hash_code);

    std::lock_guard<std::mutex> lock(StatisticContentionProfiler::cp_mutex_);
    if (sample_map_.size() == 0) {
      sample_map_[hask_key] = sc_ptr;
    } else {
      auto iter = sample_map_.find(hask_key);
      if (iter != sample_map_.end()) {
        int ret_cmp = 0;
        ret_cmp = memcmp(iter->second->stack_, sc_ptr->stack_, sizeof(void*) * sc_ptr->nframes_);
        if (ret_cmp != 0) {
          TRPC_LOG_ERROR("!!! please check, mutex: " << tmp_stream.str() << ", ret_cmp: " << ret_cmp
                                                     << ", tid: " << syscall(__NR_gettid));
        }
        iter->second->count_ += sc_ptr->count_;
        iter->second->duration_ns_ += sc_ptr->duration_ns_;
      } else {
        sample_map_[hask_key] = sc_ptr;
      }
    }
    minfo.valid = false;
  }
  return rc;
}

#ifdef TRPC_ENABLE_PROFILER
extern "C" {
int pthread_mutex_lock(pthread_mutex_t* mutex) { return TrpcPthreadMutexLockImpl(mutex); }
int pthread_mutex_unlock(pthread_mutex_t* mutex) { return TrpcPthreadMutexUnlockImpl(mutex); }
}
#endif

//////////////////////////////////////////////////////////////////////////////////
SampledContention::~SampledContention() {
  if (nframes_) {
    char** string_buff = backtrace_symbols(stack_, nframes_);
    if (string_buff) {
      free(string_buff);
    }
  }
}

size_t SampledContention::HashCode() const {
  if (nframes_ == 0) {
    return 0;
  }
  uint32_t code = 1;
  uint32_t seed = nframes_;
  MurmurHash3X86And32(stack_, sizeof(void*) * nframes_, seed, &code);
  return code;
}

////////////////////////////////////////////////////////////////////////////////////
ContentionProfiler::ContentionProfiler(const char* name) : filename_(name) {}

bool ContentionProfiler::CheckSysCallInit() {
  bool inited = true;
  InitSysMutexLock();

  std::lock_guard<std::mutex> lock(StatisticContentionProfiler::cp_mutex_);
  if (StatisticContentionProfiler::CheckContProfObj()) {
    inited = false;
  }
  return inited;
}

void ContentionProfiler::FlushTodisk(const std::string& fname,
                                     const std::map<std::string, std::shared_ptr<SampledContention>>& tmp_map) {
  TRPC_LOG_ERROR("!!! FlushTodisk, sample_map_ size: " << tmp_map.size());
  std::string content = "--- contention\n";
  for (auto& ss : tmp_map) {
    std::stringstream tmp_stream;
    tmp_stream << ss.second->duration_ns_ << " " << ss.second->count_ << " @";
    for (int i = 2; i < ss.second->nframes_; ++i) {
      tmp_stream << " " << reinterpret_cast<void*>(ss.second->stack_[i]);
    }
    tmp_stream << "\n";
    content.append(tmp_stream.str());
  }

  // read /proc/self/maps
  FILE* fp;
  char line[2048];
  fp = fopen("/proc/self/maps", "r");
  if (fp == nullptr) {
    TRPC_LOG_ERROR("opening file: /proc/self/maps fail");
  } else {
    while (fgets(line, 2048, fp) != nullptr) {
      content.append(line);
    }
    fclose(fp);
  }

  fp = fopen(fname.c_str(), "w");
  if (nullptr == fp) {
    TRPC_LOG_ERROR("In FlushTodisk, fopen fail, errno: " << errno);
    return;
  }
  if (fwrite(content.data(), content.size(), 1UL, fp) != 1UL) {
    TRPC_LOG_ERROR("In FlushTodisk, fwrite fail, errno: " << errno);
  }
  fclose(fp);
  return;
}

///////////////////////////////////////////////////////////////////////////////////
bool ContentionProfilerStart(const char* fname) {
  TRPC_LOG_ERROR("In ContentionProfilerStart entry");
  std::lock_guard<std::mutex> lock(StatisticContentionProfiler::cp_mutex_);
  if (!StatisticContentionProfiler::CheckContProfObj()) {
    TRPC_LOG_ERROR("In ContentionProfilerStart, cp_obj_ != nullptr");
    return false;
  }
  ContentionProfiler* cont_prof = new ContentionProfiler(fname);
  StatisticContentionProfiler::cp_obj_ = cont_prof;
  ++StatisticContentionProfiler::cp_version_;

  return true;
}

void ContentionProfilerStop() {
  std::lock_guard<std::mutex> lock(StatisticContentionProfiler::cp_mutex_);
  if (!StatisticContentionProfiler::CheckContProfObj()) {
    TRPC_LOG_ERROR("ContentionProfilerStop, cp_obj_ != nullptr, do FlushTodisk and reset cp_obj_");
    std::string fname = StatisticContentionProfiler::cp_obj_->filename_;
    std::map<std::string, std::shared_ptr<SampledContention>> tmp_map;
    tmp_map = std::move(sample_map_);

    delete StatisticContentionProfiler::cp_obj_;
    StatisticContentionProfiler::cp_obj_ = nullptr;

    ContentionProfiler::FlushTodisk(fname, tmp_map);
    TRPC_LOG_ERROR("ContentionProfilerStop, cp_obj_ has set nullptr and flush to dish");
  }
}

#undef BIG_CONSTANT

}  // namespace trpc::admin
