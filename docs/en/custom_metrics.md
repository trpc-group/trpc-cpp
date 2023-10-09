[中文](../zh/custom_metrics.md)

[TOC]

# Overview

Different monitoring systems have variations in data formats and reporting methods. In order to allow users to utilize different monitoring systems in a unified manner and abstract away the underlying differences, tRPC-Cpp provides a layer of abstraction for Metrics plugins. It defines the organization format for metrics data and the basic capabilities that need to be provided.

This document introduces the interface definition, usage, and development process of Metrics plugins in tRPC-Cpp.

# Plugin Specification

Metrics plugins must follow the interface definition of [trpc::Metrics](../../trpc/metrics/metrics.h) and implement the logic for inter-module reporting (ModuleReport), single-dimensional attribute reporting (SingleAttrReport), and multi-dimensional attributes reporting (MultiAttrReport).

## Plugin Interface

### ModuleReport

**ModuleReport refers to the reporting of metrics data for RPC invocations between modules. It includes information about the caller module, the called module, and the invocation result.** It can be further categorized into two types: caller reporting (for tracking client-side invocation information) and callee reporting (for tracking server-side invocation information).

The interface definition for ModuleReport is as follows:
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

`ModuleMetricsInfo` encapsulates the unified format of inter-module metrics data, where different Metrics plugins can set different fields as needed. `Metrics::ModuleReport` is used to execute the actual reporting logic of the plugin.

Notes:

* The `source` field of `ModuleMetricsInfo` is used to differentiate between client-side reporting and server-side reporting, where 0 indicates client-side reporting and 1 indicates server-side reporting.
* The `infos` field of `ModuleMetricsInfo` can record a series of key-value pairs, allowing the recording of information such as the environment, service, and address of the caller/callee.
* The tRPC-Cpp defines the most common fields as much as possible. However, due to significant differences in metrics data among different monitoring systems, there may be some data that cannot be recorded using the fields defined in `ModuleMetricsInfo`. Such information can be recorded using the `extend_info` field.

### AttributeReport

**AttributeReport refers to the monitoring and reporting of observable data by the framework and users, in addition to the inter-module metrics data reported during RPC calls.**

It includes two types of attribute reporting: single-dimensional attribute reporting(SingleAttrReport) and multi-dimensional attributes reporting(MultiAttrReport). The main differences between them are as follows:

| Type | Number of labels | Number of statistical values |
| ------ | ------ | ------ |
| SingleAttrReport | Only one | Only one |
| MultiAttrReport | One or more | One or more |

If we only consider the definition, single-dimensional attribute reporting can be seen as a special case of multi-dimensional attributes reporting. However, when implementing the reporting logic, it is possible to simplify the handling of single-dimensional attribute data. Therefore, we provides interfaces for both types of reporting.

#### Common data structure

The reporting of single-dimensional and multi-dimensional attributes involves the use of special data types defined in the framework. Here, we provide a unified explanation of these data types:

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

    MetricsPolicy refers to the statistical strategy for metrics data, and different Metrics plugins can implement their supported strategies based on the capabilities of the backend system.

2. `SummaryQuantiles`

    ```cpp
    using SummaryQuantiles = std::vector<std::vector<double>>;
    ```

    This type of parameter is used to specify the quantiles to be calculated. For example, in Prometheus, the Summary data type requires the specification of quantiles.

3. `HistogramBucket`

    ```cpp
    using HistogramBucket = std::vector<double>;
    ```

    This type of parameter is used to specify the intervals for data statistics. For example, in Prometheus, the Histogram data type requires the specification of distribution buckets.

#### SingleAttrReport

The interface definition for SingleAttrReport is as follows:
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

`SingleAttrMetricsInfo` encapsulates the unified format for single-dimensional attribute data, and different Metrics plugins can set different fields as needed. `Metrics::SingleAttrReport` is used to execute the actual reporting logic of the plugin.

Notes:

* Different monitoring systems have different formats for attribute labels. If a label has only one `key`, you can use either the `name` or `dimension` of `SingleAttrMetricsInfo`. If the label is a `key-value pair`, you can use both `name` and `dimension` together to form a key-value pair.

#### MultiAttrReport

The interface definition for MultiAttrReport is as follows:
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

`MultiAttrMetricsInfo` encapsulates the unified format for multi-dimensional attributes data, and different Metrics plugins can set different fields as needed. `Metrics::MultiAttrReport` is used to execute the actual reporting logic of the plugin.

Notes:

* Different monitoring systems have different formats for attribute labels. If the labels consist of a set of `keys`, you can use `dimensions` in `MultiAttrMetricsInfo`. If the labels consist of a set of `key-value pairs`, you can use `tags`.
* The values in `MultiAttrMetricsInfo` allow specifying different statistical strategies for different value. `Metrics::MultiAttrReport` will aggregate and report each value according to its specified statistical strategy.

## User usage pattern

The tRPC-Cpp encapsulates the Metrics plugins, allowing users to conveniently and uniformly utilize different monitoring systems. The main usage patterns are as follows:

### Performing ModuleReport by configuring filters

**Users simply need to add the filters for the corresponding Metrics plugin in the framework configuration file. The framework will automatically collect and report inter-module metrics data without the need for users to write code to invoke the reporting interface themselves.** Therefore, Metrics plugins also need to implement corresponding ServerFilter and ClientFilter. (Please refer to the [filter documentation](filter.md) for the principles of filter.)

Let's take Prometheus Metrics plugin as an example:

* Enable caller reporting:
    ```yaml
    client: 
      filter: 
        - prometheus
    ```

* Enable callee reporting:
    ```yaml
    server:
      filter:
        - prometheus
    ```

### Reporting attribute data using the unified interface

The framework provides a set of [unified reporting interfaces](../../trpc/metrics/trpc_metrics_report.h). **Users only need to specify the name of the Metrics plugin to report the metrics data to the corresponding monitoring system**:
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

Users only need to set `plugin_name` to the desired Metrics plugin name and then call the reporting interface provided by `trpc::metrics` to complete the metrics data reporting. When using it, please note that:

* The Metrics plugin corresponding to `plugin_name` must be registered during framework startup, otherwise the reporting will fail.
* Under normal circumstances, users will not directly call `ModuleReport`, but instead rely on the framework to automatically report through filters.

# Customizing Metrics plugin

According to the specifications of the Metrics plugin, developers need to:

* Implementing a Metrics plugin: used to report data to the monitoring system.
* Implementing two filters: collect metrics data for both the caller and callee during RPC calls, and invoke the Metrics plugin for reporting.
* Registering the Metrics plugin and its corresponding filters.

## Implementing Metrics plugin

The Metrics plugin needs to inherit [trpc::Metrics](../../trpc/metrics/metrics.h). And the interfaces that must be overridden are as follows:

| Interface | Description | Note |
| ------ | ------ | ------ |
| Name | Return the plugin name | It must be unique and cannot be the same as other plugin names. |
| ModuleReport | Reporting the inter-module metrics data | Refer to the section on [ModuleReport](#modulereport) for details, and ensure that the interface supports safe multi-threaded calls. |
| SingleAttrReport | Reporting the single-dimensional attribute metrics data | Refer to the section on [SingleAttrReport](#singleattrreport) for details, and ensure that the interface supports safe multi-threaded calls. |
| MultiAttrReport | Reporting the multi-dimensional attributes metrics data | Refer to the section on [MultiAttrReport](#multiattrreport) for details, and ensure that the interface supports safe multi-threaded calls. |

## Implementing filters

**How to implement filters**: please refer to the [filters documentation](filter.md) for details. **It is important to note that the name returned by the filters' `Name` interface must match the name of the plugin.**

**How to retrieve inter-module metrics data**: the information such as caller module, callee module, and invocation results can be obtained from the `Context` and framework configuration. For specific examples, please refer to the implementation of the Prometheus monitoring plugin.

**How to choose the filter points**:

1. The client filter can choose `CLIENT_PRE_RPC_INVOKE` and `CLIENT_POST_RPC_INVOKE` as filter points.

    * They can obtain all the information about the current invocation and calculate the duration from the initiation of the RPC call to its completion.
    * It is necessary to invoke the Metrics plugin's `ModuleReport` interface to report the caller data when reaching the `CLIENT_POST_RPC_INVOKE` hook.
    * The `CLIENT_POST_RPC_INVOKE` hook will also be executed during one-way calls, and no special handling is required.

2. The server filter can choose `SERVER_POST_RECV_MSG` and `SERVER_PRE_SEND_MSG` as filter points.

    * They can obtain all the information about the current invocation and calculate the duration from the moment the request packet is received to the moment the response packet is about to be sent by the framework.
    * It is necessary to invoke the Metrics plugin's `ModuleReport` interface to report the caller data when reaching the `SERVER_PRE_SEND_MSG` hook.
    * The `SERVER_PRE_SEND_MSG` hook will also be executed during one-way calls, and no special handling is required.

## Registering the plugin and filters

The interface for registering Metrics plugin:
```cpp
using MetricsPtr = RefPtr<Metrics>;

class TrpcPlugin {
 public:
  /// @brief Register custom metrics plugin
  bool RegisterMetrics(const MetricsPtr& metrics);
};
```

The interfaces for registering filters:
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

Let's illustrate with an example. Assuming that we implement a TestMetrics plugin, along with TestServerFilter and TestClientFilter filters.

1. For server scenarios, users need to register in the `TrpcApp::RegisterPlugins` function during service startup:
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

2. For pure client scenarios, registration should be done after initializing the framework configuration but before starting other framework modules:
    ```cpp
    int main(int argc, char* argv[]) {
      ParseClientConfig(argc, argv);

      TrpcPlugin::GetInstance()->RegisterMetrics(MakeRefCounted<TestMetrics>());
      TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<TestClientFilter>());

      return ::trpc::RunInTrpcRuntime([]() { return Run(); });
    }
    ```

**Note that the registered plugins and filters only need to be constructed. The framework will automatically invoke their `Init` function during startup for initialization.**
