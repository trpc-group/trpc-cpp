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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace trpc::overload_control {

/// @brief Implementation of a simple request hit queue.
/// @note 1.Implement a circular queue using vector<int64_t> with a size of N.
///       2.The size of the circular queue represents how many time slots 1s is divided into.
///       3.Each time slot corresponds to an integer element in the queue, which stores the total number of
///         accumulated requests in that time slot.
///       4.The passage of time corresponds to the movement of the "sliding window".
///          Each time the timer is triggered (once every 1/N seconds), the oldest time slot is removed and a new time
///          slot is added (with the count reset to 0).
///       5.The number of requests in the current time slot is accumulated in the count of the most recent time slot.
///       6.Calculate the total number of requests in 1s, the counts within the current queue range are accumulated.
///       7.The rotation of time slots is achieved by calling the NextFrame() function through an external timer thread.
class HitQueue {
 public:
  explicit HitQueue(int num_fps);

  virtual ~HitQueue();

  /// @brief Move to the next time slot.
  /// @note The passage of time corresponds to the movement of the "sliding window":
  ///       1.Each time the timer is triggered (once every 1/N seconds), the oldest time slot at the beginning of the
  ///         queue is removed. A new time slot is added to the end of the queue with a count of 0.
  ///       2.Update the variable for the current total number of hits.
  void NextFrame();

  /// @brief Add one to the count of valid hits.Add 1 to the count within the most recent time slot.
  /// @return The current total number of hits, after a successful addition.
  int64_t AddHit();

  /// @brief Return the current total number of hits.
  /// @return Current total number of hits.
  int64_t ActiveSum() const;

 private:
  // Get the length of the window.
  int WindowSize() const { return frame_hits_.size() - kRedundantFramesNum; }

  // Get the next index position after the current index, in a circular queue manner.
  int NextIndex(int idx) const { return (idx + 1) % frame_hits_.size(); }

  // Number of request hits within each time slot.
  std::vector<std::atomic<int64_t>> frame_hits_;
  // Starting position of the sliding window.
  std::atomic<int> window_begin_ = 0;
  // A circular queue has one extra redundant position compared to the window length, which is used to avoid
  // competition during the sliding process.
  static constexpr int kRedundantFramesNum = 1;
};

using HitQueuePtr = std::shared_ptr<HitQueue>;

}  // namespace trpc::overload_control

#endif
