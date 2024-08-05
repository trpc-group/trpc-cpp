#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
#pragma once
#include<atomic>
#include<vector>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

#include "trpc/util/function.h"



namespace trpc::overload_control{
class TickTime 
{

public:
 /// @brief 创建一个具有指定超时间隔和回调函数的定时器，并在创建后立即自动激活定时器。
 /// @param timeout_us 定时器回调触发的时间间隔（以微秒为单位）。
 /// @param on_timer 在预定时间到达时调用的回调函数。

 explicit TickTime(std::chrono::microseconds timeout_us, Function<void()>&& on_timer);

 virtual ~TickTime();

 void Deactivate();

 void Destroy();

 void Stop();

 bool Init();

private:
  void Loop();

 private:

  const std::chrono::microseconds timeout_us_;
  
  /// @param 回调函数 定时调用NextFrame()
  const Function<void()> on_timer_;
  
  /// @param 控制着loop
  std::atomic<bool> active_;
  /// @param 单独开辟线程 执行回调函数的任务
  std::thread timer_thread_;
};
}

#endif