#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL
#pragma once

#include "trpc/overload_control/smooth_filter/server_overload_controller.h"
#include "trpc/util/function.h"
#include "trpc/overload_control/smooth_filter/server_request_roll_queue.h"
#include "trpc/overload_control/smooth_filter/server_trick_time.h"

#include<string>
#include <cstdint>


/// @brief 第一步要实现一个存储request数量的时间队列
/// @note 我们用一个队列代表一个时间窗口即1s,用队列的数量N代表N份时间槽
/// 优化1 我们建议N的大小为2^n - 2,这样频繁取余操作可以用与操作去代替,有考虑过直接给用户扩容到2^n - 2
///     实际情况可能会出现扩容所占内存比较大，所以这个地方只是一个建议
/// 优化2 我们添加一个atomic变量来表示当前滑动窗口的总数，可以节省每次都计算for循环 (还没实现 牵扯到并发控制)
/// 队列本身用atomic来表示一些类内元素，保证单个变量修改操作的原子性 不是严格同步
/// 但是调用这个队列的checkpoint()时候 通过双循环来保证不会有一瞬间超过阈值

/// 第二步 插件需要一个定时器来定时处理推动窗口的前移动
/// 这里的定时器要开辟一个线程去做 所以需要线程的创立启动 线程的销毁 接口

/// 1. 使用大小为N的vector<uint64_t>实现环形队列 
/// 2. 环形队列的大小表示1秒被分成的时间槽数
/// 3. 每个时间槽对应队列中的一个整数元素，存储该时间槽中的累计请求数量
/// 4. 时间的流逝对应“滑动窗口”的移动
/// 每次定时器触发（每1/N秒一次），最旧的时间槽被移除并添加一个新的时间槽（计数重置为0）
/// 5. 当前时间槽中的请求数量累积到最新的时间槽的计数中
/// 6. 计算1秒内的总请求数时，累积当前队列范围内的计数
/// 7. 时间槽的旋转通过外部定时器线程调用NextFrame()函数实现

namespace trpc::overload_control 
{
/// @var: 默认的限流数量 
constexpr int32_t kDefaultNum = 100;

class SmoothLimit : public ServerOverloadController
{
public:
  /// @brief 初始化滑动窗口限流插件
  /// @param 插件的名字 限流数量 是否记录监控日志 时间槽数量
  explicit SmoothLimit(std::string name,int64_t limit, bool is_report = false, int32_t window_size = kDefaultNum);

  /// @note 什么也不干 整个插件需要手动销毁的资源仅仅是线程资源
  /// @brief 从filter的实现来看 销毁之前会先调用插件的stop()和destroy()
  ~SmoothLimit();

  /// @note 埋点位置调用onrequest()之前调用的func 内部调用checkpoint()即可 
  /// @param context代表上下文 里边存储这status、service name、caller name
  /// @param 根据name信息去对应factory对象（单例）可以找到对应的具体插件策略
  bool BeforeSchedule(const ServerContextPtr& context);

  /// @note 此处埋点无需调用此func
  bool AfterSchedule(const ServerContextPtr& context){return true;}

  std::string Name() {return name_;}
  
  ///@brief 初始化线程资源 
  bool Init();

  /// @brief 结束定时线程的loop 同时让定时线程joinable
  void Stop();

  /// @brief 销毁线程
  void Destroy() ;
public: //这个地方应该是private 我在test测试了 所以定义成了public
  ///@brief 检查是否超出窗口当前限流
  bool CheckLimit(const ServerContextPtr& check_param);

  /// @brief 获取当前窗口内发生的请求数量
  int64_t GetCurrCounter() ;

  /// @brief 获取限流阈值
  int64_t GetMaxCounter() const  { return limit_; }
public:
  //插件的名字
  std::string name_;

  //定时调用这个func 移动滑动窗口
  void OnNextTimeFrame();

  //滑动窗口限制的request数量
  int64_t limit_;

  //是否记录监控数据
  bool is_report_{false};

  //滑动窗口中的时间槽数量
  int32_t window_size_{kDefaultNum};

  //存储request的滑动窗口时间槽
  RequestRollQueue requestrollque_;

  //定时器 另起线程 每1/N调用nextframe()移动滑动窗口
  TickTime tick_timer_;

};

}
#endif

