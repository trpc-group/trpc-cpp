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
// tRPC is licensed under the Apache 2.0 License, and includes source codes from
// the following components:
// 1. incubator-brpc
// Copyright (C) 2019 The Apache Software Foundation
// incubator-brpc is licensed under the Apache 2.0 License.
//
//

#include "trpc/tvar/common/sampler.h"

#include <list>
#include <memory>
#include <thread>
#include <utility>

#include "trpc/tvar/common/atomic_type.h"
#include "trpc/tvar/common/write_mostly.h"
#include "trpc/util/chrono/time.h"
#include "trpc/util/thread/thread_helper.h"

namespace {

using SamplerPtr = std::shared_ptr<trpc::tvar::Sampler>;

constexpr int kWarnNoSleepThreshold = 2;

}  // namespace

namespace trpc::tvar {

// The following source codes are from incubator-brpc.
// Copied and modified from
// https://github.com/apache/brpc/blob/1.6.0/src/bvar/detail/sampler.cpp

/// @brief To combine two Samplers.
struct CombineSampler {
  /// @brief Insert one sampler into a list.
  void operator()(std::list<SamplerPtr>* s1, SamplerPtr s2) const {
    s1->emplace_back(std::move(s2));
  }

  /// @brief Insert one sampler list into another.
  void operator()(std::list<SamplerPtr>* s1, std::list<SamplerPtr> s2) const {
    s1->splice(s1->end(), s2);
  }
};

/// @brief Intended for WriteMostly.
struct CollectorTraits {
  using Type = std::list<SamplerPtr>;
  using WriteBuffer = AtomicType<Type>;
  using InputDataType = SamplerPtr;
  using ResultType = std::list<SamplerPtr>;
  using SamplerOp = CombineSampler;
  using SamplerInvOp = VoidOp;

  /// @brief update user data into thread local buffer.
  template <typename TLS>
  static void Update(TLS* wb, InputDataType val) {
    wb->buffer_.template Modify<CombineSampler, InputDataType>(std::move(val));
  }

  /// @brief Merge thread local data into global buffer.
  static void Merge(ResultType* wb1, Type wb2) { CombineSampler()(wb1, std::move(wb2)); }
};

/// @brief For sampler_collector thread.
class SamplerCollector : public WriteMostly<CollectorTraits> {
 public:
  SamplerCollector(SamplerCollector const&) = delete;

  void operator=(SamplerCollector const&) = delete;

  static SamplerCollector& GetInstance() {
    static SamplerCollector instance;
    return instance;
  }

  /// @brief Start sampler_collector thread.
  void Start() {
    if (thread_) {
      return;
    }
    stop_ = false;
    thread_ = std::make_unique<std::thread>([this]() { Run(); });
  }

  /// @brief Stop sampler_collector thread.
  void Stop() {
    if (stop_) {
      return;
    }
    stop_ = true;
    Join();
  }

  /// @brief Wait sampler_collector thread to stop.
  void Join() {
    if (thread_->joinable()) {
      thread_->join();
    }
    thread_ = nullptr;
  }

  ~SamplerCollector() { Stop(); }

 private:
  SamplerCollector() : WriteMostly<CollectorTraits>(std::list<SamplerPtr>(), std::list<SamplerPtr>()) {
    Start();
  }

  /// @brief Main logic of sampler_collector thread.
  void Run() {
    SetCurrentThreadName("sampler_collector");

    int consecutive_no_sleep = 0;
    while (!stop_) {
      auto abs_time = trpc::time::GetSteadyMicroSeconds();
      if (auto s = Reset(); !s.empty()) {
        root_.splice(root_.end(), s);
      }
      int removed_num = 0;
      int sampled_num = 0;

      for (auto itr = root_.begin(); itr != root_.end();) {
        // We may remove p from the list, save next first.
        std::unique_lock<std::mutex> lc((*itr)->mutex_);
        if (!(*itr)->used_) {
          lc.unlock();
          itr = root_.erase(itr);
          ++removed_num;
        } else {
          (*itr)->TakeSample();
          lc.unlock();
          ++sampled_num;
          ++itr;
        }
      }
      TRPC_FMT_TRACE("sampled_num:{}, removed_num:{}", sampled_num, removed_num);

      bool ever_sleep = false;
      auto now = trpc::time::GetSteadyMicroSeconds();
      calculated_time_us_ += now - abs_time;
      abs_time += 1000000;
      while (abs_time > now) {
        std::this_thread::sleep_for(std::chrono::microseconds(abs_time - now));
        ever_sleep = true;
        now = trpc::time::GetSteadyMicroSeconds();
      }
      if (ever_sleep) {
        consecutive_no_sleep = 0;
      } else {
        if (++consecutive_no_sleep >= kWarnNoSleepThreshold) {
          consecutive_no_sleep = 0;
          TRPC_FMT_WARN("stats is busy at sampling for {}  seconds!", kWarnNoSleepThreshold);
        }
      }
    }
  }

 private:
  bool stop_{false};
  std::unique_ptr<std::thread> thread_{nullptr};
  int64_t calculated_time_us_{0};
  std::list<SamplerPtr> root_{};
};

Sampler::Sampler() : used_(true) {}

void Sampler::Schedule() {
  SamplerCollector::GetInstance().Update(this->shared_from_this());
}

void Sampler::Destroy() {
  std::unique_lock<std::mutex> lock(mutex_);
  used_ = false;
}

void SamplerCollectorThreadStart() {
  SamplerCollector::GetInstance().Start();
}

void SamplerCollectorThreadStop() {
  SamplerCollector::GetInstance().Stop();
}

// End of source codes that are from incubator-brpc.

}  // namespace trpc::tvar
