[English](../en/tvar.md)

# 前言

`tvar` 是 tRPC-Cpp 框架提供给用户的一个多线程统计类库，辅助用户记录和跟踪程序运行过程中的各种状态。

# 设计原理

本文分析了业界支持统计类库的三个同类型框架，分别从统计类型多样性，写入效率，查询效率以及实现原理四个方面，对它们进行了对比，

| 比较维度/框架名称 | envoy | brpc | flare |
|----------------|-------|------|--------|
| 统计类型多样性 | 较多 | 较多 | 较少 |
| 写入效率 | 低 | 高 | 很高 |
| 查询效率 | 一般 | 低 | 低 |
| 实现原理 | global atomic | thread local atomic | thread local atomic |

从上表可知，envoy 的实现原理是多线程操作同一个全局原子变量，并发写入时会存在 cache bouncing，因此在写多读少场景下，性能较差，但是在读多写少场景下，性能较好。

相比之下，brpc 和 flare 的实现原理是各个线程写入自己本地私有的原子变量，从而避免了写入时的 cache bouncing，因此在写多读少场景下，性能更好，但是在读的时候，需要先对各个线程的私有变量做聚合，因此在读多写少场景下，性能会变差。

考虑到tvar是面向写多读少的场景设计的，因此同时借鉴了 brpc 和 flare 的设计方案，通过 thread_local 机制来提高写入性能，并且在统计类型多样性上，使用 brpc 的方案。

# 统计类型

目前 tvar 一共支持11种统计类型，如下表所示，

| 类型名称 | 功能 |
|---------|-----|
| Counter | 只增不减的计数器 |
| Gauge | 可增可减的计数器 |
| Maxer | 取最大值 |
| Miner | 取最小值 |
| Averager | 取平均值 |
| IntRecorder | 取平均值，只支持int类型 |
| Status | 用户传入一个值用于显示，不同线程修改后立即可见 |
| PassiveStatus | 用户传入一个函数对象，读取结果的时候调用它拿到返回值 |
| Window | 获取一段时间内的统计值 |
| PerSecond | 获取一段时间内每秒的平均统计值 |
| LatencyRecorder | 获取qps以及分位耗时 |

# 使用指南

## 配置

```yaml
global:
  tvar:
    window_size: 10            # 窗口大小，单位是秒，用于Window，PerSecond和LatencyRecorder三个统计类型
    save_series: true          # 是否存储历史数据，默认是true
    abort_on_same_path: true   # 如果发现注册的两个tvar变量有相同的曝光路径时，是否直接abort退出进程，默认是true
    latency_p1: 80             # 用户自定义分位值1，只能传1-99以内的整数，对应1%到99%
    latency_p2: 90             # 用户自定义分位值2，只能传1-99以内的整数，对应1%到99%
    latency_p3: 99             # 用户自定义分位值3，只能传1-99以内的整数，对应1%到99%
```

## 使用

### Counter

```cpp
#include <cstdint>

// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::Counter;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 通过曝光构造创建tvar变量，后续可以通过admin接口查询
  Counter<uint64_t> counter("user/my_counter");
  // 用户也可以使用不曝光构造，但是后续不能通过admin接口查询
  // Counter<uint64_t> counter;

  // 增加3
  counter.Add(3);
  // 增加1
  counter.Increment();
  // 用户一般不需要调用GetValue，而是通过admin接口查看值，这里是演示用法
  auto ret = counter.GetValue();
  return 0;
}
```

### Gauge

```cpp
#include <cstdint>

// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::Gauge;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 通过曝光构造创建tvar变量，后续可以通过admin接口查询
  Gauge<uint64_t> gauge("user/my_gauge");
  // 用户也可以使用不曝光构造，但是后续不能通过admin接口查询
  // Gauge<uint64_t> gauge;

  // 增加2
  gauge.Add(2);
  // 减少2
  gauge.Subtract(2);
  // 增加1
  gauge.Increment();
  // 减少1
  gauge.Decrement();
  // 用户一般不需要调用GetValue，而是通过admin接口查看值，这里是演示用法
  auto ret = gauge.GetValue();
  return 0;
}
```

### Maxer

```cpp
#include <cstdint>

// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::Maxer;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 通过曝光构造创建tvar变量，后续可以通过admin接口查询
  Maxer<uint64_t> maxer("user/my_maxer");
  // 用户也可以使用不曝光构造，但是后续不能通过admin接口查询
  // Maxer<uint64_t> maxer;

  // 更新到3
  maxer.Update(3);
  // 更新到9
  maxer.Update(9);
  // 更新后还是9
  maxer.Update(6);

  // 用户一般不需要调用GetValue，而是通过admin接口查看值，这里是演示用法
  auto ret = maxer.GetValue();
  return 0;
}
```

### Miner

```cpp
#include <cstdint>

// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::Miner;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 通过曝光构造创建tvar变量，后续可以通过admin接口查询
  Miner<uint64_t> miner("user/my_miner");
  // 用户也可以使用不曝光构造，但是后续不能通过admin接口查询
  // Miner<uint64_t> miner;

  // 更新到9
  miner.Update(9);
  // 更新到3
  miner.Update(3);
  // 更新后还是3
  miner.Update(6);

  // 用户一般不需要调用GetValue，而是通过admin接口查看值，这里是演示用法
  auto ret = miner.GetValue();
  return 0;
}
```

### Averager

```cpp
#include <cstdint>

// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::Averager;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 通过曝光构造创建tvar变量，后续可以通过admin接口查询
  Averager<uint64_t> averager("user/my_averager");
  // 用户也可以使用不曝光构造，但是后续不能通过admin接口查询
  // Averager<uint64_t> averager;

  // 更新后平均值是3
  averager.Update(3);
  // 更新后平均值是5
  averager.Update(7);
  // 更新后平均值是5
  averager.Update(5);

  // 用户一般不需要调用GetValue，而是通过admin接口查看值，这里是演示用法
  auto ret = averager.GetValue();
  return 0;
}
```

### IntRecorder

```cpp
#include <cstdint>

// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::IntRecorder;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 通过曝光构造创建tvar变量，后续可以通过admin接口查询
  IntRecorder int_recorder("user/my_int_recorder");
  // 用户也可以使用不曝光构造，但是后续不能通过admin接口查询
  // IntRecorder int_recorder;

  // 更新后平均值是3
  int_recorder.Update(3);
  // 更新后平均值是5
  int_recorder.Update(7);
  // 更新后平均值是5
  int_recorder.Update(5);

  // 用户一般不需要调用GetValue，而是通过admin接口查看值，这里是演示用法
  auto ret = int_recorder.GetValue();
  return 0;
}
```

### Status

```cpp
// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::Status;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 通过曝光构造创建tvar变量，后续可以通过admin接口查询
  // 如果用户使用的类型是tvar::Status<std::string>，那么单参构造是不曝光的；如果要曝光需要至少传两个参数，第一个是path，第二个是初始值
  Status<int> status("user/status", 0);
  // 用户也可以使用不曝光构造，但是后续不能通过admin接口查询
  // Status<int> status;

  // 设置值
  status.SetValue(3);

  ret = status.GetValue();
  return 0;
}
```

### PassiveStatus

```cpp
#include <cstdint>
#include <mutex>

// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::PassiveStatus;
using std::mutex;

int g_num{0};
mutex g_mutex;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 通过曝光构造创建tvar变量，后续可以通过admin接口查询
  // 用户需要自己保证回调函数的线程安全性
  PassiveStatus<int> passive_status("user/passive_status", []() {
    std::unique_lock<std::mutex> lock(g_mutex);
    return g_num;
  });
  // 用户也可以使用不曝光构造，但是后续不能通过admin接口查询
  // PassiveStatus<int> passive_status([]() {
  //   std::unique_lock<std::mutex> lock(g_mutex);
  //   return g_num;
  // });

  // 用户一般不需要调用GetValue，而是通过admin接口查看值，这里是演示用法
  auto ret = passive_status.GetValue();

  {
    std::unique_lock<std::mutex> lock(g_mutex);
    ++g_num;
  }

  ret = passive_status.GetValue();
  return 0;
}
```

### Window

```cpp
#include <cstdint>

// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::Window;
using trpc::tvar::Gauge;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 通过曝光构造创建tvar变量，后续可以通过admin接口查询
  Gauge<int64_t> gauge("user/my_gauge");
  // 用户也可以使用不曝光构造，但是后续不能通过admin接口查询
  // Gauge<int64_t> gauge;

  // 定义Window，曝光路径：/user/window，窗口大小10秒，数据来源：gauge变量
  Window<Gauge<int64_t>> window("user/window", &gauge, 10);

  // 对源变量进行更新操作
  gauge.Increment();
  gauge.Add(2);

  // 用户一般不需要调用GetValue，而是通过admin接口查看值，这里是演示用法
  auto ret = window.GetValue();

  return 0;
}
```

在使用Window时，有些类型可以支持Window，有些则不支持，具体参考下表，

| 源变量类型 | 是否支持Window | 说明 |
|----------|---------------|-----|
| 数值类型的Counter | 支持 | 无 |
| 数值类型的Gauge | 支持 | 无 |
| 数值类型的Maxer | 支持 | 语义特殊，不建议对Maxer使用Window |
| 数值类型的Miner | 支持 | 语义特殊，不建议对Miner使用Window |
| 数值类型的Averager | 支持 | 无 |
| IntRecorder | 支持 | 无 |
| Status | 不支持 | 使用Window没有意义 |
| 数值类型的PassiveStatus | 支持 | 无 |
| PerSecond | 不支持 | 本身已经是窗口语义 |
| LatencyRecorder | 不支持 | 本身已经是窗口语义 |

### PerSecond

```cpp
#include <cstdint>

// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::PerSecond;
using trpc::tvar::Gauge;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 通过曝光构造创建tvar变量，后续可以通过admin接口查询
  Gauge<int64_t> gauge("user/my_gauge");
  // 用户也可以使用不曝光构造，但是后续不能通过admin接口查询
  // Gauge<int64_t> gauge;

  // 定义PerSecond，曝光路径：/user/per_second，窗口大小10秒，数据来源：gauge变量
  PerSecond<Gauge<int64_t>> per_second("user/per_second", &gauge, 10);

  // 对源变量进行更新操作
  gauge.Increment();
  gauge.Add(2);

  // 用户一般不需要调用GetValue，而是通过admin接口查看值，这里是演示用法
  auto ret = per_second.GetValue();

  return 0;
}
```

在使用PerSecond时，有些类型可以支持PerSecond，有些则不支持，具体参考下表，

| 源变量类型 | 是否支持PerSecond | 说明 |
|----------|------------------|-----|
| 数值类型的Counter | 支持 | 无 |
| 数值类型的Gauge | 支持 | 无 |
| 数值类型的Maxer | 不支持 | 使用PerSecond没有意义 |
| 数值类型的Miner | 不支持 | 使用PerSecond没有意义 |
| 数值类型的Averager | 不支持 | 使用PerSecond没有意义 |
| IntRecorder | 不支持 | 使用PerSecond没有意义 |
| Status | 不支持 | 使用PerSecond没有意义 |
| 数值类型的PassiveStatus | 支持 | 无 |
| Window | 不支持 | 本身用于实现PerSecond |
| LatencyRecorder | 不支持 | 本身已经是窗口语义 |

### LatencyRecorder

```cpp
// 包含头文件
#include "trpc/tvar/tvar.h"

using trpc::tvar::LatencyRecorder;

int main() {
  // 这里以局部变量举例，实际开发中，推荐使用全局单例或者其他合适的生命周期

  // 定义LatencyRecorder，曝光路径：/user/latency_recorder，窗口大小10秒
  LatencyRecorder latency_recorder("user/latency_recorder");

  // 输入延迟
  latency_recorder.Update(3);

  // 用户一般不需要调用Count和LatencyPercentile，而是通过admin接口查看值，这里是演示用法
  auto qps = latency_recorder.Count();
  auto p99 = latency_recorder.LatencyPercentile(0.99);
  return 0;
}
```

LatencyRecorder统计的信息及其含义如下表所示，

| 字段名称 | 含义 |
|--------|------|
| latency | 获取当前窗口下平均值耗时 |
| max_latency | 获取当前窗口下面最大耗时 |
| count | 获取历史输入的总数量，这里的数量是用户通过Update()输入的个数 |
| qps | 获取当前窗口下的qps |
| latency_9999 | 当前窗口下的99.99分位耗时；即p9999，只有0.01%的耗时比这个数值高 |
| latency_999 | 当前窗口下的99.9分位耗时；即p999，只有0.1%的耗时比这个数值高 |
| latency_p1 | 默认值80，即p80；用户可以通过配置latency_p1自定义分位 |
| latency_p2 | 默认值90，即p90；用户可以通过配置latency_p2自定义分位 |
| latency_p3 | 默认值99，即p99；用户可以通过配置latency_p3自定义分位 |

## 查询

### 查询当前值

```bash
curl http://admin_ip:admin_port/cmds/var/{path}
```

其中，path是tvar变量的路径，可以查询某个具体的tvar变量，也可以查询某个曝光路径下的所有tvar变量，举例如下，

```cpp
Gauge<int64_t> gauge("user/b/gauge");
gauge.Add(2);
Counter<uint64_t> count("user/counter");
count.Add(1);
```

查询某个具体的tvar变量gauge，

```bash
curl http://admin_ip:admin_port/cmds/var/user/b/gauge
```

查询得到的结果如下，

```bash
{
    "guage":2
}
```

查询某个曝光路径下的所有tvar变量，包括gauge变量和count变量，

```bash
curl http://admin_ip:admin_port/cmds/var/user
```

查询得到的结果如下，

```bash
{
    "counter":1,
    "b":{
        "guage":2
    }
}
```

### 查询历史值

```bash
curl http://admin_ip:admin_port/cmds/var/{path}?history=true
```

在查询当前值的基础上，在末尾添加query string `history=true`

以上文的gauge变量举例，查询方式如下，

```bash
curl http://admin_ip:admin_port/cmds/var/user/b/gauge?history=true
```

查询得到的结果如下，

```bash
{
"gauge":{
        "latest_day":[1,2,3,4],
        "latest_hour":[1,2,3,4],
        "latest_min":[1,2,3,4],
        "latest_sec":[1,2,3,4],
        "now":[1]
    }
}
```

有些类型支持历史值查询，有些类型不支持，具体如下表，

| 源变量类型 | 是否支持历史值查询 | 说明 |
|----------|-----------------|-----|
| 数值类型的Counter | 支持 | 无 |
| 数值类型的Gauge | 支持 | 无 |
| 数值类型的Maxer | 不支持 | Maxer如果不重置的话，没办法采集到1秒内的数据，建议使用Window<数值类型的Maxer> |
| 数值类型的Miner | 不支持 | Miner如果不重置的话，没办法采集到1秒内的数据，建议使用Window<数值类型的Miner> |
| 数值类型的Averager | 不支持 | 没办法采集到1秒内的数据，建议使用Window<数值类型的Averager> |
| IntRecorder | 不支持 | 没办法采集到1秒内的数据，建议使用Window&lt;IntRecorder> |
| 数值类型的Status | 支持 | 无 |
| Window<数值类型的Counter>	| 支持 | 无 |
| Window<数值类型的Gauge> | 支持 | 无 |
| 数值类型的PassiveStatus | 支持 | 无 |
| Window<数值类型的Maxer>| 支持 | 无 |
| Window<数值类型的Miner> | 支持 | 无 |
| Window&lt;IntRecorder> | 支持 | 无 |
| Window<数值类型的PassiveStatus> | 支持 | 无 |
| PerSecond<数值类型的Counter> | 支持 | 无 |
| PerSecond<数值类型的Gauge> | 支持 | 无 |
| PerSecond&lt;PassiveStatus> | 支持 | 无 |
