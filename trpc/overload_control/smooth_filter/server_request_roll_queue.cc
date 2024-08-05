#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/smooth_filter/server_request_roll_queue.h"

#include <numeric>

namespace trpc::overload_control {

RequestRollQueue::RequestRollQueue(int num_fps) : time_hits_(num_fps + kRedundantFramesNum) {
  //这里先构造构造一个指定时间槽数量的vec，再给每个计数赋值为0
  //因为是写入 赋值的时候可以用release内存序 比默认强度轻量级一些
  for (std::atomic<int64_t>& hit_num : time_hits_) {
    hit_num.store(0, std::memory_order_release);
  }
}

RequestRollQueue::~RequestRollQueue() {}

void RequestRollQueue::NextTimeFrame() {
  //读入内存序问题同上
  //先拿到当前窗口起始位置 
  //注意 这一系列动作并不是同步的
  const int begin = begin_pos_.load(std::memory_order_acquire);
  
  const int end = isPowOfTwo_ ? (begin + WindowSize()) & (time_hits_.size() - 1) 
    : (begin + WindowSize()) % time_hits_.size();
  

  // 最后一个窗口槽计数置为空
  time_hits_.at(end).store(0, std::memory_order_release);
  
  // 改变窗口起始位置
  begin_pos_.store(NextIndex(begin), std::memory_order_release);
}

int64_t RequestRollQueue::AddTimeHit() {
  //给最后一个槽计数+1在返回当前的总计数
  const int begin = begin_pos_.load(std::memory_order_acquire);
  const int current_frame = isPowOfTwo_ ? (begin + WindowSize() - 1) & (time_hits_.size() - 1) 
    : (begin + WindowSize() - 1) % time_hits_.size();
  time_hits_.at(current_frame).fetch_add(1, std::memory_order_release);
  return ActiveSum();
}

int64_t RequestRollQueue::ActiveSum() const {
  const int begin = begin_pos_.load(std::memory_order_acquire);
  const int end = (begin + WindowSize()) % time_hits_.size();
  int64_t sum = 0;
  for (int i = begin; i != end; i = NextIndex(i)) {
    sum += time_hits_.at(i).load(std::memory_order_acquire);
  }
  return sum;
}

}  // namespace trpc::overload_control
#endif