[English](../en/custom_metrics.md)

# 前言

不同监控系统的数据格式、上报方式之间存在差异。为了让用户可以用统一的方式去使用不同的监控系统，屏蔽底层的差异，tRPC-Cpp对监控插件做了一层抽象，规定了监控数据的组织格式，以及需要提供的基本能力。

这篇文档介绍tRPC-Cpp中监控插件的接口定义、使用方式，以及开发者如何开发一个监控插件。

# 插件规范

监控插件必须遵循[trpc::Metrics](../../trpc/metrics/metrics.h)的接口定义，实现模调监控上报（ModuleReport）、单维属性上报（SingleAttrReport）和多维属性上报（MultiAttrReport）的逻辑。

## 插件接口

### 模调上报

**模调上报（ModuleReport）是指RPC模块间的调用监控数据上报，上报主调模块的信息、被调模块的信息、调用结果等。** 其又分为主调监控上报（统计客户端调用信息）和被调监控上报（统计服务端调用信息）两种类型。

模调上报的接口定义如下：
```cpp
constexpr int kMetricsCallerSource = 0;
constexpr int kMetricsCalleeSource = 1;

/// @brief Metrics data for inter-module calls
struct ModuleMetricsInfo {
  /// Reporting source, where kMetricsCallerSource indicates reporting from the caller and kMetricsCalleeSource
  /// indicates reporting from the callee
  int source;

  /// Statistical information
  std::map<std::string, std::string> infos;

  /// Call's status, where 0 indicates success, 1 indicates exception, and 2 indicates timeout
  int state;

  /// Call's status code of framework
  int frame_ret_code;

  /// Call's status code of user interface
  int interface_ret_code;

  /// Call's duration
  uint32_t cost_time;

  /// Call's protocol
  std::string protocol;

  /// Extend information, can be used to provide more information when need
  std::any extend_info;
};

class Metrics : public Plugin {
 public:
  /// @brief Reports the metrics data for inter-module calls
  /// @param info metrics data
  /// @return the reported result, where 0 indicates success and non-zero indicates failure
  virtual int ModuleReport(const ModuleMetricsInfo& info) = 0;
  virtual int ModuleReport(ModuleMetricsInfo&& info) { return ModuleReport(info); }
};
```

`ModuleMetricsInfo`封装了模调监控数据的统一格式，不同监控插件根据需要对不同的字段进行设置即可。`Metrics::ModuleReport`用于执行插件实际的上报逻辑。

需要注意：

* `ModuleMetricsInfo`的`source`用于区分是主调上报还是被调上报，规定`0`表示主调，`1`表达被调。
* `ModuleMetricsInfo`的`infos`可以记录一系列的键值对，主/被调的环境、服务、地址等信息均可以通过该字段来记录。
* 框架尽可能地对最通用的字段进行定义。但由于不同监控系统的监控数据存在较大差异，可能存在一些无法使用`ModuleMetricsInfo`确定类型的字段进行记录的数据，这些信息可以通过`extend_info`来记录。

### 属性上报

**属性上报是指除了在RPC调用时上报的模调监控数据外，框架和用户对其他可观察数据进行监控上报。** 

其包括单维属性上报和多维属性上报两种，它们的主要区别如下：

| 类型 | 标签数量 | 统计值数量 |
| ------ | ------ | ------ |
| 单维属性上报 | 只有一个 | 只有一个 |
| 多维属性上报 | 可以有多个 | 可以有多个 |

如果只考虑定义，可以把单维属性上报当成是多维属性上报的一种特殊情况。但在实现上报逻辑时可以考虑对单维属性数据进行简化处理，所以框架同时提供这两种类型的上报接口。

#### 公用数据结构

单维属性上报和多维属性上报会用到一些框架定义的特殊数据类型，在这里统一进行说明：

1. `MetricsPolicy`

    ```cpp
    /// @brief Statistical strategy
    enum MetricsPolicy {
        /// Overwrite the statistical value
        SET = 0,
        /// Calculate the sum value
        SUM = 1,
        /// Calculate the mean value
        AVG = 2,
        /// Calculate the maximum value
        MAX = 3,
        /// Calculate the minimum value
        MIN = 4,
        /// Calculate the median value
        MID = 5,
        /// Calculate the quartiles
        QUANTILES = 6,
        /// Calculate the histogram
        HISTOGRAM = 7,
    };
    ```

    MetricsPolicy是指监控数据的统计策略，不同监控插件根据后端系统的能力实现能支持的策略即可。

2. `SummaryQuantiles`

    ```cpp
    using SummaryQuantiles = std::vector<std::vector<double>>;
    ```

    该类型的参数用于指定要统计的分位数。例如Prometheus中Summary类型数据需要指定的分位数。

3. `HistogramBucket`

    ```cpp
    using HistogramBucket = std::vector<double>;
    ```

    该类型的参数用于指定数据统计的区间。例如Prometheus中Histogram类型数据需要指定的分布桶。

#### 单维属性上报

单维属性上报的接口定义如下：
```cpp
/// @brief Metrics data with single-dimensional attribute
struct SingleAttrMetricsInfo {
  /// Metrics name
  std::string name;

  /// Metrics dimension
  std::string dimension;

  /// Metrics policy
  MetricsPolicy policy;

  /// Statistical value
  double value;

  /// The bucket used to gather histogram statistics
  HistogramBucket bucket;

  /// The quantiles used to gather summary statistics
  SummaryQuantiles quantiles;
};

class Metrics : public Plugin {
 public:
  /// @brief Reports the metrics data with single-dimensional attribute
  /// @param info metrics data
  /// @return the reported result, where 0 indicates success and non-zero indicates failure
  virtual int SingleAttrReport(const SingleAttrMetricsInfo& info) = 0;
  virtual int SingleAttrReport(SingleAttrMetricsInfo&& info) { return SingleAttrReport(info); }
};
```

`SingleAttrMetricsInfo`封装了单维属性数据的统一格式，不同监控插件根据需要对不同的字段进行设置即可。`Metrics::SingleAttrReport`用于执行插件实际的上报逻辑。

需要注意：

* 不同的监控系统的属性标签格式不同。如果标签只有一个`key`，那么可以使用`SingleAttrMetricsInfo`的`name`或者`dimension`；如果标签是一个键值对，那么可以同时使用`name`和`dimension`来构成一个键值对。

#### 多维属性上报

多维属性上报的接口定义如下：
```cpp
/// @brief Metrics data with multi-dimensional attributes
struct MultiAttrMetricsInfo {
  /// Metrics name
  std::string name;

  /// Metrics tags
  std::map<std::string, std::string> tags;

  /// Metrics dimensions
  std::vector<std::string> dimensions;

  /// Statistical values
  std::vector<std::pair<MetricsPolicy, double>> values;

  /// The bucket used to gather histogram statistics
  HistogramBucket bucket;

  /// The quantiles used to gather summary statistics
  SummaryQuantiles quantiles;
};

class Metrics : public Plugin {
 public:
  /// @brief Reports the metrics data with multi-dimensional attributes
  /// @param info metrics data
  /// @return the reported result, where 0 indicates success and non-zero indicates failure
  virtual int MultiAttrReport(const MultiAttrMetricsInfo& info) = 0;
  virtual int MultiAttrReport(MultiAttrMetricsInfo&& info) { return MultiAttrReport(info); }
};
```

`MultiAttrMetricsInfo`封装了多维属性数据的统一格式，不同监控插件根据需要对不同的字段进行设置即可。`Metrics::MultiAttrReport`用于执行插件实际的上报逻辑。

需要注意：

* 不同的监控系统的属性标签格式不同。如果标签由一组`key`组成，那么可以使用`MultiAttrMetricsInfo`的`dimensions`；如果标签由一组键值对组成，那么可以使用`tags`。
* `MultiAttrMetricsInfo`的`values`，不同统计值可以指定不同的统计策略，`Metrics::MultiAttrReport`会对每个统计值都按照其指定的统计策略进行汇聚上报。

## 用户使用方式

框架对监控插件进行了封装，使得用户可以方便、统一的方式去使用不同的监控系统，主要的使用方式如下：

### 以配置拦截器的方式进行模调上报

**用户只需在框架配置文件中加上对应监控插件的拦截器，即可由框架自动采集并上报模调数据，而不需要自行写代码调用模调上报接口。** 因此监控插件也需要实现对应的服务端拦截器和客户端拦截器。（拦截器的原理请参考[拦截器说明文档](filter.md)）

以Prometheus监控插件为例：

* 启用主调上报：
    ```yaml
    client: 
      filter: 
        - prometheus
    ```

* 启用被调上报：
    ```yaml
    server:
      filter:
        - prometheus
    ```

### 使用统一的接口上报属性数据

框架提供了一组[统一的上报接口](../../trpc/metrics/trpc_metrics_report.h)，**用户只需要指定要上报的监控插件名，即可将监控数据上报到对应的监控系统** ：
```cpp
/// @brief Trpc metrics data for inter-module calls
struct TrpcModuleMetricsInfo {
  /// Metrics plugin name. Specifies which plugin to report the data to.
  std::string plugin_name;
  /// Metrics data
  ModuleMetricsInfo module_info;
};

/// @brief Trpc metrics data with single-dimensional attribute
struct TrpcSingleAttrMetricsInfo {
  /// Metrics plugin name. Specifies which plugin to report the data to.
  std::string plugin_name;
  /// Metrics data
  SingleAttrMetricsInfo single_attr_info;
};

/// @brief Trpc metrics data with multi-dimensional attributes
struct TrpcMultiAttrMetricsInfo {
  /// Metrics plugin name.  Specifies which plugin to report the data to.
  std::string plugin_name;
  /// Metrics data
  MultiAttrMetricsInfo multi_attr_info;
};

/// @brief Metrics interfaces for user programing
namespace metrics {

/// @brief Report the trpc metrics data for inter-module calls
/// @param info metrics data
/// @return the reported result, where 0 indicates success and non-zero indicates failure
int ModuleReport(const TrpcModuleMetricsInfo& info);
int ModuleReport(TrpcModuleMetricsInfo&& info);

/// @brief Report the trpc metrics data with single-dimensional attribute
/// @param info metrics data
/// @return the reported result, where 0 indicates success and non-zero indicates failure
int SingleAttrReport(const TrpcSingleAttrMetricsInfo& info);
int SingleAttrReport(TrpcSingleAttrMetricsInfo&& info);

/// @brief Report the metrics data with multi-dimensional attributes
/// @param info metrics data
/// @return the reported result, where 0 indicates success and non-zero indicates failure
int MultiAttrReport(const TrpcMultiAttrMetricsInfo& info);
int MultiAttrReport(TrpcMultiAttrMetricsInfo&& info);

}  // namespace metrics
```

用户只需将`plugin_name`设置为要上报的监控插件名，然后调用`trpc::metrics`提供的上报接口即可完成监控数据上报。使用时需要注意：

* `plugin_name`对应的监控插件必须在框架启动时注册，不然上报会失败。
* 正常情况下，用户不会直接调用`ModuleReport`，而是以拦截器的方式由框架自动去上报。

# 自定义监控插件

根据监控插件的规范，开发者需要：

* 实现一个监控插件：用于把数据上报到监控系统。
* 实现两个拦截器：在RPC调用过程中收集主/被调监控数据，调用监控插件上报。
* 注册监控插件和对应的拦截器。

## 实现监控插件

监控插件需要继承[trpc::Metrics](../../trpc/metrics/metrics.h)。必须重写如下接口：

| 接口 | 说明 | 备注 |
| ------ | ------ | ------ |
| Name | 返回插件名 | 必须保证唯一，不能和其他插件名相同 |
| ModuleReport | 模调监控上报 | 详见[模调上报](#模调上报)小节，必须保证多线程调用是安全的 |
| SingleAttrReport | 单维属性上报 | 详见[单维属性上报](#单维属性上报)小节，必须保证多线程调用是安全的 |
| MultiAttrReport | 多维属性上报 | 详见[多维属性上报](#多维属性上报)小节，必须保证多线程调用是安全的 |

## 实现拦截器

拦截器的实现：具体参考[拦截器说明文档](filter.md)。**需要注意拦截器`Name`接口返回的名字必须与插件的名字一致。**

模调数据的获取：主调模块信息、被调模块信息、调用结果等可以从`Context`和框架配置中获取，具体例子可以参考[Prometheus监控插件实现](../../trpc/metrics/prometheus/)。

埋点的选择：

1. 客户端拦截器可以选择：`CLIENT_PRE_RPC_INVOKE`和`CLIENT_POST_RPC_INVOKE`。

    * 可以拿到当前调用的所有信息，以及统计出框架从发起RPC调用到调用完成的耗时。
    * 需要在执行到`CLIENT_POST_RPC_INVOKE`埋点时，调用监控插件的`ModuleReport`接口上报主调数据。
    * 单向调用时`CLIENT_POST_RPC_INVOKE`埋点也会被执行到，不需要特殊处理。

2. 服务端拦截器可以选择：`SERVER_POST_RECV_MSG`和`SERVER_PRE_SEND_MSG`。

    * 可以拿到当前调用的所有信息，以及统计出框架从刚收到请求包到即将发送响应包的耗时。
    * 需要在执行到`SERVER_PRE_SEND_MSG`埋点时，调用监控插件的`ModuleReport`接口上报被调数据。
    * 单向调用时`SERVER_PRE_SEND_MSG`埋点也会被执行到，不需要特殊处理。

## 注册插件和拦截器

插件注册的接口：
```cpp
using MetricsPtr = RefPtr<Metrics>;

class TrpcPlugin {
 public:
  /// @brief Register custom metrics plugin
  bool RegisterMetrics(const MetricsPtr& metrics);
};
```

拦截器注册的接口：
```cpp
using MessageServerFilterPtr = std::shared_ptr<MessageServerFilter>;
using MessageClientFilterPtr = std::shared_ptr<MessageClientFilter>;

class TrpcPlugin {
 public:
  /// @brief Register custom server filter
  bool RegisterServerFilter(const MessageServerFilterPtr& filter);

  /// @brief Register custom client filter
  bool RegisterClientFilter(const MessageClientFilterPtr& filter);
};
```

举个例子进行说明。假设自定义了一个TestMetrics插件，TestServerFilter、TestClientFilter两个拦截器，那么

1. 对于服务端场景，用户需要在服务启动的`TrpcApp::RegisterPlugins`函数中注册：
    ```cpp
    class HelloworldServer : public ::trpc::TrpcApp {
     public:
      ...
      int RegisterPlugins() override {
        TrpcPlugin::GetInstance()->RegisterMetrics(MakeRefCounted<TestMetrics>());
        TrpcPlugin::GetInstance()->RegisterServerFilter(std::make_shared<TestServerFilter>());
        TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<TestClientFilter>());
        return 0;
      }
    };
    ```

2. 对于纯客户端场景，需要在启动框架配置初始化后，框架其他模块启动前注册：
    ```cpp
    int main(int argc, char* argv[]) {
      ParseClientConfig(argc, argv);

      TrpcPlugin::GetInstance()->RegisterMetrics(MakeRefCounted<TestMetrics>());
      TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<TestClientFilter>());

      return ::trpc::RunInTrpcRuntime([]() { return Run(); });
    }
    ```

**注意注册的插件和拦截器只需要构造即可，框架启动时会自动调用它们的`Init`函数进行初始化。**
