[中文版](../zh/tvar.md)

[TOC]

# tRPC-Cpp tvar user guide

## Preface

Tvar is a multi-threaded statistics library provided by the tRPC-Cpp framework to assist users in recording and tracking various states during program execution.

## Design approach

This article analyzes three similar frameworks in the industry that support statistics libraries. It compares them in terms of the diversity of statistical types, write efficiency, query efficiency, and implementation principle,

| dimensions/framework | envoy | brpc | flare |
|----------------------|-------|------|-------|
| diversity of statistical types | rich | rich | poor |
| write efficiency | low | middle | high |
| query efficiency | middle | low | low |
| implementation principle | global atomic | thread local atomic | thread local atomic |

From the table above, it can be observed that the implementation principle of Envoy involves multiple threads operating on a shared global atomic variable. However, concurrent write operations may lead to cache bouncing, resulting in poor performance in scenarios with frequent writes and fewer reads. On the other hand, Envoy performs well in scenarios with frequent reads and fewer writes.

In comparison, the implementation principle of brpc and flare involve each thread writing to its own local private atomic variable, thereby avoiding cache bouncing during write operations. As a result, they exhibit better performance in scenarios with frequent writes and fewer reads. However, during read operations, aggregation of private variables from each thread is required, which leads to decreased performance in scenarios with frequent reads and fewer writes.

Considering that tvar is designed for scenarios with frequent writes and fewer reads, it incorporates design elements from both brpc and flare. It leverages the thread_local mechanism to improve write performance and adopts the approach used by brpc for diverse statistical types.

## Statistical types

Currently, tvar supports a total of 11 statistical types, as shown in the table below,

| type | function |
|------|----------|
| Counter | An increment-only counter. |
| Gauge | An increment-decrement counter. |
| Maxer | Record Maximum value. |
| Miner | Record Miner value. |
| Averager | Record average value. |
| IntRecorder | Record average value, only support int type value. |
| Status | User provides value for display, immediately visible after modification by different threads. |
| PassiveStatus | User provides a function object, which is invoked to obtain the returned value when reading the result. |
| Window | Obtain statistical values within a certain time period. |
| PerSecond | Obtain the average statistical values per second within a certain time period. |
| LatencyRecorder | Obtain QPS (Queries Per Second) and percentile latency. |

## User guide

### Configutation

```yaml
global:
  tvar:
    window_size: 10           # Window size, measured in seconds, used for the Window, PerSecond, and LatencyRecorder statistical types.
    save_series: true         # Whether to store historical data, default is true.
    abort_on_same_path: true  # Whether to directly abort and exit the process if two registered tvar variables are found to have the same exposed path, default is true.
    latency_p1: 80            # User-defined percentile value 1, can only be an integer within 1-99, corresponding to 1% to 99%.
    latency_p2: 90            # User-defined percentile value 2, can only be an integer within 1-99, corresponding to 1% to 99%.
    latency_p3: 99            # User-defined percentile value 3, can only be an integer within 1-99, corresponding to 1% to 99%.
```

### Usage

#### Counter

```cpp
#include <cstdint>

// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::Counter;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Create tvar variable through exposed construction, which can be queried through admin commands.
  Counter<uint64_t> counter("user/my_counter");
  // Users can also use non-exposed construction, but they cannot query it through admin commands later.
  // Counter<uint64_t> counter;

  // Add 3.
  counter.Add(3);
  // Add 1.
  counter.Increment();
  // Users generally do not need to call GetValue directly, but rather view the value through admin commands. This is a demonstration usage.
  auto ret = counter.GetValue();
  return 0;
}
```

#### Gauge

```cpp
#include <cstdint>

// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::Gauge;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Create tvar variable through exposed construction, which can be queried through admin commands.
  Gauge<uint64_t> gauge("user/my_gauge");
  // Users can also use non-exposed construction, but they cannot query it through admin commands later.
  // Gauge<uint64_t> gauge;

  // Add 2.
  gauge.Add(2);
  // Minus 2.
  gauge.Subtract(2);
  // Add 1.
  gauge.Increment();
  // Minus 1.
  gauge.Decrement();
  // Users generally do not need to call GetValue directly, but rather view the value through admin commands. This is a demonstration usage.
  auto ret = gauge.GetValue();
  return 0;
}
```

#### Maxer

```cpp
#include <cstdint>

// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::Maxer;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Create tvar variable through exposed construction, which can be queried through admin commands.
  Maxer<uint64_t> maxer("user/my_maxer");
  // Users can also use non-exposed construction, but they cannot query it through admin commands later.
  // Maxer<uint64_t> maxer;

  // Update to 3.
  maxer.Update(3);
  // Update to 9.
  maxer.Update(9);
  // Still 9 after this update.
  maxer.Update(6);

  // Users generally do not need to call GetValue directly, but rather view the value through admin commands. This is a demonstration usage.
  auto ret = maxer.GetValue();
  return 0;
}
```

#### Miner

```cpp
#include <cstdint>

// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::Miner;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Create tvar variable through exposed construction, which can be queried through admin commands.
  Miner<uint64_t> miner("user/my_miner");
  // Users can also use non-exposed construction, but they cannot query it through admin commands later.
  // Miner<uint64_t> miner;

  // Update to 9.
  miner.Update(9);
  // Update to 3.
  miner.Update(3);
  // Still 3 after this update.
  miner.Update(6);

  // Users generally do not need to call GetValue directly, but rather view the value through admin commands. This is a demonstration usage.
  auto ret = miner.GetValue();
  return 0;
}
```

#### Averager

```cpp
#include <cstdint>

// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::Averager;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Create tvar variable through exposed construction, which can be queried through admin commands.
  Averager<uint64_t> averager("user/my_averager");
  // Users can also use non-exposed construction, but they cannot query it through admin commands later.
  // Averager<uint64_t> averager;

  // Average value equal to 3 after this update.
  averager.Update(3);
  // Average value equal to 5 after this update.
  averager.Update(7);
  // Average value equal to 5 after this update.
  averager.Update(5);

  // Users generally do not need to call GetValue directly, but rather view the value through admin commands. This is a demonstration usage.
  auto ret = averager.GetValue();
  return 0;
}
```

#### IntRecorder

```cpp
#include <cstdint>

// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::IntRecorder;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Create tvar variable through exposed construction, which can be queried through admin commands.
  IntRecorder int_recorder("user/my_int_recorder");
  // Users can also use non-exposed construction, but they cannot query it through admin commands later.
  // IntRecorder int_recorder;

  // Average value equal to 3 after this update.
  int_recorder.Update(3);
  // Average value equal to 3 after this update.
  int_recorder.Update(7);
  // Average value equal to 3 after this update.
  int_recorder.Update(5);

  // Users generally do not need to call GetValue directly, but rather view the value through admin commands. This is a demonstration usage.
  auto ret = int_recorder.GetValue();
  return 0;
}
```

#### Status

```cpp
// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::Status;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Create tvar variable through exposed construction, which can be queried through admin commands.
  // If user is using the type tvar::Status<std::string>, the single-parameter constructor is non-exposure. To expose it, at least two parameters need to be passed, the first one is the path, and the second one is the initial value.
  Status<int> status("user/status", 0);
  // Users can also use non-exposed construction, but they cannot query it through admin commands later.
  // Status<int> status;

  // Set value.
  status.SetValue(3);

  ret = status.GetValue();
  return 0;
}
```

#### PassiveStatus

```cpp
#include <cstdint>
#include <mutex>

// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::PassiveStatus;
using std::mutex;

int g_num{0};
mutex g_mutex;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Create tvar variable through exposed construction, which can be queried through admin commands.
  // User needs to ensure the thread safety of their callback functions.
  PassiveStatus<int> passive_status("user/passive_status", []() {
    std::unique_lock<std::mutex> lock(g_mutex);
    return g_num;
  });
  // Users can also use non-exposed construction, but they cannot query it through admin commands later.
  // PassiveStatus<int> passive_status([]() {
  //   std::unique_lock<std::mutex> lock(g_mutex);
  //   return g_num;
  // });

  // Users generally do not need to call GetValue directly, but rather view the value through admin commands. This is a demonstration usage.
  auto ret = passive_status.GetValue();

  {
    std::unique_lock<std::mutex> lock(g_mutex);
    ++g_num;
  }

  ret = passive_status.GetValue();
  return 0;
}
```

#### Window

```cpp
#include <cstdint>

// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::Window;
using trpc::tvar::Gauge;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Create tvar variable through exposed construction, which can be queried through admin commands.
  Gauge<int64_t> gauge("user/my_gauge");
  // Users can also use non-exposed construction, but they cannot query it through admin commands later.
  // Gauge<int64_t> gauge;

  // Define a Window with an exposure path of "/user/window", a window size of 10 seconds, and data source from previous gauge variable.
  Window<Gauge<int64_t>> window("user/window", &gauge, 10);

  // Update source variable.
  gauge.Increment();
  gauge.Add(2);

  // Users generally do not need to call GetValue directly, but rather view the value through admin commands. This is a demonstration usage.
  auto ret = window.GetValue();

  return 0;
}
```

When using Window, some types can support it while others cannot. Please refer to the table below for specific details.

| type of source variable | support Window or not | illustration |
|-------------------------|-----------------------|--------------|
| Counter with numeric type | yes | void |
| Gauge with numeric type | yes | void |
| Maxer with numeric type | yes | Due to its special semantics, it is not recommended to use Window with Maxer. |
| Miner with numeric type | yes | Due to its special semantics, it is not recommended to use Window with Miner. |
| Averager with numeric type | yes | void |
| IntRecorder | yes | void |
| Status | no | Make no sense to use Window. |
| PassiveStatus with numeric type | yes | void |
| PerSecond | no | Window semantics already. |
| LatencyRecorder | no | Window semantics already. |

#### PerSecond

```cpp
#include <cstdint>

// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::PerSecond;
using trpc::tvar::Gauge;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Create tvar variable through exposed construction, which can be queried through admin commands.
  Gauge<int64_t> gauge("user/my_gauge");
  // Users can also use non-exposed construction, but they cannot query it through admin commands later.
  // Gauge<int64_t> gauge;

  // Define a PerSecond with an exposure path of "/user/per_second", a window size of 10 seconds, and data source from previous gauge variable.
  PerSecond<Gauge<int64_t>> per_second("user/per_second", &gauge, 10);

  // Update source variable.
  gauge.Increment();
  gauge.Add(2);

  // Users generally do not need to call GetValue directly, but rather view the value through admin commands. This is a demonstration usage.
  auto ret = per_second.GetValue();

  return 0;
}
```

When using PerSecond, some types can support it while others cannot. Please refer to the table below for specific details.

| type of source variable | support PerSecond or not | illustration |
|-------------------------|--------------------------|--------------|
| Counter with numeric type | yes | void |
| Gauge with numeric type | yes | void |
| Maxer with numeric type | no | Make no sense to use PerSecond. |
| Miner with numeric type | no | Make no sense to use PerSecond. |
| Averager with numeric type | no | Make no sense to use PerSecond. |
| IntRecorder | no | Make no sense to use PerSecond. |
| Status | no | Make no sense to use PerSecond. |
| PassiveStatus with numeric type | yes | void |
| Window | no | Used to implement PerSecond. |
| LatencyRecorder | no | Window semantics already. |

#### LatencyRecorder

```cpp
// Include header files.
#include "trpc/tvar/tvar.h"

using trpc::tvar::LatencyRecorder;

int main() {
  // Here, local variables are used as an example. In actual development, it is recommended to use global singletons or other appropriate lifecycles.

  // Define a LatencyRecorder with an exposure path of "/user/latency_recorder", a window size of 10 seconds
  LatencyRecorder latency_recorder("user/latency_recorder");

  // Input latency.
  latency_recorder.Update(3);

  // Users generally do not need to call GetValue directly, but rather view the value through admin commands. This is a demonstration usage.
  auto qps = latency_recorder.Count();
  auto p99 = latency_recorder.LatencyPercentile(0.99);
  return 0;
}
```

The information and their meanings recorded by LatencyRecorder are as shown in the table below,

| field name | meanings |
|------------|----------|
| latency | Get the average latency within the current window. |
| max_latency | Get the max latency within the current window. |
| count | Retrieve the total count of historical inputs, where the count represents the number of inputs provided by the user through the Update() function. |
| qps | Get the qps within the current window. |
| latency_9999 | The duration of the 99.99th percentile within the current window, also known as p9999, represents the threshold above which only 0.01% of durations exceed. |
| latency_999 | The duration of the 99.9th percentile within the current window, also known as p999, represents the threshold above which only 0.1% of durations exceed. |
| latency_p1 | The default value is 80, which corresponds to p80. Users can customize the percentile by configuring latency_p1. |
| latency_p2 | The default value is 90, which corresponds to p90. Users can customize the percentile by configuring latency_p2. |
| latency_p3 | The default value is 99, which corresponds to p99. Users can customize the percentile by configuring latency_p3. |

### Query

#### Query current value

```bash
curl http://admin_ip:admin_port/cmds/var/{path}
```

The "path" refers to the path of a tvar variable. It can be used to query a specific tvar variable or to query all tvar variables under a specific exposed path. Here is the example,

```cpp
Gauge<int64_t> gauge("user/b/gauge");
gauge.Add(2);
Counter<uint64_t> count("user/counter");
count.Add(1);
```

Querying a specific tvar variable "gauge".

```bash
curl http://admin_ip:admin_port/cmds/var/user/b/gauge
```

The obtained result is as follows,

```bash
{
    "guage":2
}
```

Querying all tvar variables under a specific exposure path, including "gauge" variables and "count" variables.

```bash
curl http://admin_ip:admin_port/cmds/var/user
```

The obtained result is as follows,

```bash
{
    "counter":1,
    "b":{
        "guage":2
    }
}
```

#### Query history value

```bash
curl http://admin_ip:admin_port/cmds/var/{path}?history=true
```

On top of querying the current value, append the query string `history=true` at the end.

Using the example of the "gauge" variable mentioned earlier, the query would be as follows,

```bash
curl http://admin_ip:admin_port/cmds/var/user/b/gauge?history=true
```

The obtained result is as follows,

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

Some types support historical value queries, while others do not. The details are provided in the following table,

| type of source variable | support historical value or not | illustration |
|-------------------------|---------------------------------|--------------|
| Counter with numeric type | yes | void |
| Gauge with numeric type | yes | voud |
| Maxer with numeric type | no | If Maxer is not reset, it is not possible to collect data within 1 second. It is recommended to use Window<Maxer with numeric type>. |
| Miner with numeric type | no | If Miner is not reset, it is not possible to collect data within 1 second. It is recommended to use Window<Miner with numeric type>. |
| Averager with numeric type | no | It is impossible to collect data within 1 second. It is recommended to use Window<Averager with numeric type>. |
| IntRecorder | no | It is impossible to collect data within 1 second. It is recommended to use Window<IntRecorder>. |
| Status with numeric type | yes | void |
| Window<Counter with numeric type>	 | yes | void |
| Window<Gauge with numeric type> | yes | void |
| PassiveStatus with numeric type | yes | void |
| Window<Maxer with numeric type>	| yes | void |
| Window<Miner with numeric type> | yes | void |
| Window<IntRecorder> | yes | void |
| Window<PassiveStatus with numeric type> | yes | void |
| PerSecond<Counter with numeric type> | yes | void |
| PerSecond<Gauge with numeric type> | yes | void |
| PerSecond<PassiveStatus> | yes | void |
