#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
#pragma once

#include<atomic>
#include<vector>
#include <memory>
#include "trpc/util/log/logging.h"

/// 整个队列并没有做到严格的同步
/// 我们通过原子变量来保证单步操作的原子性 选定内存序做优化
/// 调用这个队列的checkpoint()时候 通过双循环来保证不会有一瞬间超过阈值

namespace trpc::overload_control{
class RequestRollQueue
{
public:
  explicit RequestRollQueue(int n_fps);

  virtual ~RequestRollQueue();

  /// @brief 移动窗口
  void NextTimeFrame();

  /// @brief end添加时间数
  int64_t AddTimeHit();

  /// @brief 获取smooth窗口的total_size
  int64_t ActiveSum() const;  

private:
 int WindowSize() const { return time_hits_.size() - kRedundantFramesNum; }
 
 //begin + 1
 int NextIndex(int idx) const { return isPowOfTwo_ ? (idx + 1) & (time_hits_.size() - 1) : (idx + 1) % time_hits_.size(); }
private:
 //记录起始位置
 std::atomic<int> begin_pos_ = 0;

 std::atomic<int> current_limit_ = 0;

 //时间槽队列
 std::vector<std::atomic<int64_t>> time_hits_;

 //冗余位置 判断队空队满的时候
 static constexpr int kRedundantFramesNum = 1;

 //判断是否能优化取余操作
 bool isPowOfTwo_;

};
}
#endif