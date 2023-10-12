[中文](../zh/prometheus_metrics.md)

# Overview

The tRPC-Cpp encapsulates some functionalities of [Prometheus](https://prometheus.io/), enabling the collection of prometheus metrics data within the framework and providing the ability to export metrics data externally.

This document introduces how to use Prometheus in tRPC-Cpp. Developers can learn the following topics:

* The way to enable prometheus.
* The functionalities provided by the Prometheus Metrics plugin:
  * ModuleReport
  * AttributeReport
* The way to report custom data using the Prometheus native API.
* The way to export metrics data.

# Enable prometheus

By default, tRPC-Cpp framework does not compile related code with prometheus. To enable it, users need to add the relevant compilation options during program compilation.

## Bazel

Add the `"trpc_include_prometheus"` compilation option during Bazel compilation.

For example, add it in `.bazelrc` file.

```sh
build --define trpc_include_prometheus=true
```

## CMake

Add the `TRPC_BUILD_WITH_METRICS_PROMETHEUS` compilation option during CMake compilation.

For example:

```cmake
cmake -DTRPC_BUILD_WITH_METRICS_PROMETHEUS=ON -DCMAKE_BUILD_TYPE=Release ..
```

# Prometheus Metrics plugin

The tRPC-Cpp provides a Metrics plugin based on Prometheus, which follows the framework's [Metrics interface](../../trpc/metrics/metrics.h) and offers capabilities for reporting inter-module and attribute data.

## Plugin configuration

As long as the related compilation option of prometheus are enabled during compilation, the framework will automatically register the Prometheus Metrics plugin upon startup. The parameters of the plugin can be configured in the framework's configuration file. If you not configured, the default parameters will be used.

```yaml
plugins:
  metrics:
    prometheus:
      histogram_module_cfg:
        - 1
        - 10
        - 100
        - 1000
      const_labels:
        key1: value1
        key2: value2
```

The description of the configuration item are as follow.

| Configuration options | Type | Value | Description |
| ------ | ------ | ------ | ------ |
| histogram_module_cfg | Sequences | No, the default value is [1, 10, 100, 1000] | Statistical interval for latency distribution in ModuleReport, measured in milliseconds. |
| const_labels | Mappings | No, the default value is empty. | Default labels attached to each RPC statistical data. |

## ModuleReport

**ModuleReport refers to reporting metrics data of RPC inter-module calls, including caller reporting (for tracking client-side invocation information) and callee reporting (for tracking server-side invocation information).**

### Caller reporting

Simply add the `prometheus` filter to the `client` section of the framework's configuration file to enable client-side monitoring.

```yaml
client:
  ...
  filter:
    - prometheus
```

**After enabling, the framework will automatically collect RPC call data of the caller.**

Statistical data:

| Metrics Name | Metrics Type | Description |
| ------ | ------ | ------ |
| rpc_client_counter_metric | Counter | Total number of calls initiated by the client |
| rpc_client_histogram_metric | Histogram | Distribution of client-side call latency. (unit: ms) |

Statistical labels:

| Key | Value |
| ------ | ------ |
| physicEnv | namespace |
| userEnv | env |
| aApp | Name of the caller app |
| aServer | Name of the caller server |
| aService | Name of the caller service |
| pApp | Name of the callee app |
| pServer | Name of the callee server |
| pService | Name of the callee service |
| pInterface | Name of the callee interface |
| pIp | IP address of the callee |
| pContainer | Name of the callee container |
| pConSetId | Set to which the callee belongs |
| frame_ret_code | Framework error code |
| interface_ret_code | Interface error code |

### Callee reporting

Simply add the `prometheus` filter to the `server` section of the framework's configuration file to enable client-side monitoring.

```yaml
server:
  ...
  filter:
    - prometheus
```

**After enabling, the framework will automatically collect RPC call data of the callee.**

Statistical data:

```mermaid
| Metrics Name | Metrics Type | Description |
| ------ | ------ | ------ |
| rpc_server_counter_metric | Counter | Total number of requests received by the server |
| rpc_server_histogram_metric | Histogram | Distribution of request processing time of the server (unit: ms) |

Statistical labels:

| Key | Value |
| ------ | ------ |
| physicEnv | namespace |
| userEnv | env |
| aApp | Name of the caller app |
| aServer | Name of the caller server |
| aService | Name of the caller service |
| aIp | IP address of the caller |
| pApp | Name of the callee app |
| pServer | Name of the callee server |
| pService | Name of the callee service |
| pInterface | Name of the callee interface |
| pIp | IP address of the callee |
| pContainer | Name of the callee container |
| pConSetId | Set to which the callee belongs |
| frame_ret_code | Framework error code |
| interface_ret_code | Interface error code |
```

## AttributeReport

### Attribute metrics items

In addition to automatically collecting RPC call data, the plugin also defines a set of attribute metrics items internally,allowing users to collect and analyze other required data.

| Metrics Name | Type |
| ------ | ------ |
| prometheus_counter_metric | Counter |
| prometheus_gauge_metric | Gauge |
| prometheus_summary_metric | Summary |
| prometheus_histogram_metric | Histogram |

### Statistical strategies

The statistical strategies provided by the plugin are as follow.

| Statistical Strategy | Corresponding Metrics Item | Description |
| ------ | ------ | ------ |
| ::trpc::MetricsPolicy::SET | prometheus_gauge_metric | Set the value, monitor the changes in values. |
| ::trpc::MetricsPolicy::SUM | prometheus_counter_metric | Calculate the cumulative count of the data. |
| ::trpc::MetricsPolicy::MID | prometheus_summary_metric | Calculate the median value of the data. |
| ::trpc::MetricsPolicy::QUANTILES | prometheus_summary_metric | Calculate the specific quantile value of statistical data. |
| ::trpc::MetricsPolicy::HISTOGRAM | prometheus_histogram_metric | Calculate the interval distribution of statistical data. |

### Type description

Some special data types defined by frameworks are used for attribute reporting, and they will be explained here collectively.

1. `SummaryQuantiles`

    This type of parameter is used to specify the quantiles to be calculated and is used in `QUANTILES` statistics. The actual type is `std::vector<std::vector<double>>`, corresponding to `Summary::Quantiles` in Prometheus.

    Its assignment cannot be empty, and each quantile must consist of two values. The first value represents the quantile to be calculated, and the second value represents the acceptable error range. For example, to calculate the 0.5 quantile with an acceptable error of 0.05 and the 0.9 quantile with an acceptable error of 0.1, it can be set as: `{{0.5, 0.05}, {0.9, 0.1}}`.

2. `HistogramBucket`

    This type of parameter is used to specify the intervals for data statistics and is used in `HISTOGRAM` statistics. The actual type is `std::vector<double>`, corresponding to `Histogram::BucketBoundaries` in Prometheus.

### Report methods

For detailed usage examples, please refer to the [Prometheus example](../../examples/features/prometheus/proxy/).

#### Report with convenient plugin interface

The Prometheus Metrics plugin provides a set of convenient reporting interfaces that adapt data for Prometheus. Users can report data without the need to construct complex data structures as required by the general single-dimensional or multi-dimensional attribute reporting interfaces. **It is recommended for users to utilize these interfaces.**

1. The API file

    In order to use this set of interfaces, you need to first import the `Prometheus API` file from tRPC-Cpp.

    ```cpp
    #include "trpc/metrics/prometheus/prometheus_metrics_api.h"
    ```

2. Report the data with type `SET`

    ```cpp
    namespace trpc::prometheus {
    
    /// @brief Reports metrics data with SET type
    /// @param labels prometheus metrics labels
    /// @param value the value to set
    /// @return report result, where 0 indicates success and non-zero indicates failure
    int ReportSetMetricsInfo(const std::map<std::string, std::string>& labels, double value);
    
    }
    ```

3. Report the data with type `SUM`

    ```cpp
    namespace trpc::prometheus {
    
    /// @brief Reports metrics data with SUM type
    /// @param labels prometheus metrics labels
    /// @param value the value to increment
    /// @return report result, where 0 indicates success and non-zero indicates failure
    int ReportSumMetricsInfo(const std::map<std::string, std::string>& labels, double value);

    }
    ```

4. Report the data with type `MID`

    ```cpp
    namespace trpc::prometheus {

    /// @brief Reports metrics data with MID type
    /// @param labels prometheus metrics labels
    /// @param value the value to observe
    /// @return report result, where 0 indicates success and non-zero indicates failure
    int ReportMidMetricsInfo(const std::map<std::string, std::string>& labels, double value);

    }
    ```

5. Report the data with type `QUANTILES`

    ```cpp
    namespace trpc::prometheus {

    /// @brief Reports metrics data with QUANTILES type
    /// @param labels prometheus metrics labels
    /// @param quantiles the quantiles used to gather summary statistics
    /// @param value the value to observe
    /// @return report result, where 0 indicates success and non-zero indicates failure
    int ReportQuantilesMetricsInfo(const std::map<std::string, std::string>& labels, const SummaryQuantiles& quantiles,
                                    double value);

    }
    ```

6. Report the data with type `HISTOGRAM`

    ```cpp
    namespace trpc::prometheus {

    /// @brief Reports metrics data with HISTOGRAM type
    /// @param labels prometheus metrics labels
    /// @param bucket the bucket used to gather histogram statistics
    /// @param value the value to observe
    /// @return report result, where 0 indicates success and non-zero indicates failure
    int ReportHistogramMetricsInfo(const std::map<std::string, std::string>& labels, const HistogramBucket& bucket,
                                    double value);
    int ReportHistogramMetricsInfo(const std::map<std::string, std::string>& labels, HistogramBucket&& bucket,
                                    double value);

    }
    ```

#### Report with common SingleAttrReport interface

The Prometheus Metrics plugin supports the framework's general single-dimensional attribute reporting method, which involves constructing `::trpc::TrpcSingleAttrMetricsInfo` and using the `::trpc::metrics::SingleAttrReport` interface for reporting. **The single-dimensional attribute reporting of Prometheus refers to reporting data where the statistical label contains only one key-value pair.**

When setting the value of `::trpc::TrpcSingleAttrMetricsInfo`, please note that:

1. `plugin_name` must be specified as the name of the Prometheus Metrics plugin: `::trpc::prometheus::kPrometheusMetricsName`.

2. `single_attr_info.policy` must be correctly set to the desired statistical policy.

3. `single_attr_info.name` represents the key of the label, while `single_attr_info.dimension` represents the value of the label.

4. `single_attr_info.value` is the value to be observed and statistically analyzed.

5. `single_attr_info.quantiles` and `single_attr_info.bucket` need to be set when reporting data of type `QUANTILES` and `HISTOGRAM`, respectively.

Taking the reporting of data with type `SUM` as an example.

```cpp
#include "trpc/metrics/prometheus/prometheus_metrics_api.h"

::trpc::TrpcSingleAttrMetricsInfo single_metrics_info;
single_metrics_info.plugin_name = ::trpc::prometheus::kPrometheusMetricsName;
single_metrics_info.single_attr_info.policy = ::trpc::MetricsPolicy::SUM;
single_metrics_info.single_attr_info.name = "single_test_key";
single_metrics_info.single_attr_info.dimension = "single_test_value";
single_metrics_info.single_attr_info.value = 1;
::trpc::metrics::SingleAttrReport(std::move(single_metrics_info));
```

#### Report with common MultiAttrReport interface

The Prometheus Metrics plugin supports the framework's general multi-dimensional attribute reporting method, which involves constructing `::trpc::TrpcMultiAttrMetricsInfo` and using the `::trpc::metrics::MultiAttrReport` interface for reporting. **The multi-dimensional attribute reporting of Prometheus refers to reporting data where the statistical label contains multiple key-value pairs.**

When setting the value of `::trpc::TrpcMultiAttrMetricsInfo`, please note that:

1. `plugin_name` must be specified as the name of the Prometheus Metrics plugin: `::trpc::prometheus::kPrometheusMetricsName`.

2. Set `multi_attr_info.tags` as the corresponding collection of key-value pairs.

3. `multi_attr_info.values` can only have one pair value, where pair.first is set to the desired statistical strategy, and pair.second is set to the value to be observed and statistically analyzed.

4. `multi_attr_info.quantiles` and `multi_attr_info.bucket` need to be set when reporting data of type `QUANTILES` and `HISTOGRAM`, respectively.

Taking the reporting of data with type `SUM` as an example.

```cpp
#include "trpc/metrics/prometheus/prometheus_metrics_api.h"

::trpc::TrpcMultiAttrMetricsInfo multi_metrics_info;
multi_metrics_info.plugin_name = ::trpc::prometheus::kPrometheusMetricsName;
multi_metrics_info.multi_attr_info.values = {{::trpc::MetricsPolicy::SUM, 1}};
multi_metrics_info.multi_attr_info.tags = {{"multi_test_key1", "multi_test_value1"}, {"multi_test_key2", "multi_test_value2"}};
::trpc::metrics::MultiAttrReport(std::move(multi_metrics_info));
```

# Customize metrics items

Users can refer to the native [Prometheus](https://github.com/jupp0r/prometheus-cpp) API to customize their own metrics items and metrics data, without being limited to the metrics items and strategies provided by the framework.

The framework provides interfaces for obtaining prometheus metrics items, allowing users to report their own metrics data once they have obtained the metrics family. The retrieval interface is as follows:

```cpp
namespace trpc::prometheus {

/// @brief Gets a counter type monitoring family.
::prometheus::Family<::prometheus::Counter>* GetCounterFamily(const char* name, const char* help,
                                                              const std::map<std::string, std::string>& labels = {});
/// @brief Gets a gauge type monitoring family.
::prometheus::Family<::prometheus::Gauge>* GetGaugeFamily(const char* name, const char* help,
                                                          const std::map<std::string, std::string>& labels = {});

/// @brief Gets a histogram type monitoring family.
::prometheus::Family<::prometheus::Histogram>* GetHistogramFamily(
    const char* name, const char* help, const std::map<std::string, std::string>& labels = {});

/// @brief Gets a summary type monitoring family.
::prometheus::Family<::prometheus::Summary>* GetSummaryFamily(const char* name, const char* help,
                                                              const std::map<std::string, std::string>& labels = {});

}  // namespace trpc::prometheus
```

Taking the example of retrieving a metrics item of type `Counter` and reporting it.

```cpp
#include "trpc/metrics/prometheus/prometheus_metrics_api.h"

::prometheus::Family<::prometheus::Counter>* counter_family = ::trpc::prometheus::GetCounterFamily(
      "counter_name", "counter_desc", {{"const_counter_key", "const_counter_value"}});
::prometheus::Counter& counter = counter_family->Add({{"counter_key", "counter_value"}});
counter.Increment(1);
```

# Export metrics data

In tRPC-Cpp framework, Prometheus is only responsible for collecting metrics data and does not provide the functionality to actively push metrics data. However, the framework provides external interfaces to directly access the collected prometheus data. Additionally, if the service has enabled the admin feature, metrics data can also be obtained by accessing the admin interface.

## Retrieve by interface

The framework provides a `Collect` interface to directly access the raw prometheus data, allowing users to decide how to utilize the metrics data.

```cpp
namespace trpc::prometheus {

/// @brief Gets monitoring data collected by Prometheus.
std::vector<::prometheus::MetricFamily> Collect();

}  // namespace trpc::prometheus
```

Similarly, in order to use this interface, you need to include the `trpc/metrics/prometheus/prometheus_metrics_api.h` file.

## Retrieve by admin

If the service has enabled the admin feature, you can access the prometheus data which serialized as a string by visiting `http://admin_ip:admin_port/metrics`.
