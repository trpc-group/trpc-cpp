[English](../en/prometheus_metrics.md)

# 前言

tRPC-Cpp 框架对 [Prometheus](https://prometheus.io/)的部分功能进行了封装，实现了在框架内采集 Prometheus 监控数据，并对外提供监控数据导出的能力。

这篇文档介绍如何在 tRPC-Cpp 框架中使用 Prometheus，开发者可以了解到如下内容：

* 如何开启 Prometheus 监控
* Prometheus 监控插件提供的能力：
  * 模调监控上报
  * 属性监控上报
* 如何使用 Prometheus 原生 API 上报自定义数据
* 如何导出监控数据

# 开启 Prometheus 监控

tRPC-Cpp 框架默认不会编译 Prometheus 相关的代码。若要开启，用户需要在程序编译时加上相关的编译选项。

## Bazel 启用方式

bazel 编译时加上`“trpc_include_prometheus”`编译选项。

例如在 `.bazelrc` 中加上：

```sh
build --define trpc_include_prometheus=true
```

## CMake 启用方式

cmake 编译时加上 `TRPC_BUILD_WITH_METRICS_PROMETHEUS` 编译选项。

例如：

```cmake
cmake -DTRPC_BUILD_WITH_METRICS_PROMETHEUS=ON -DCMAKE_BUILD_TYPE=Release ..
```

# Prometheus 监控插件

tRPC-Cpp 框架基于 Prometheus 提供了一个 Metrics 插件，其遵循框架规定的 [Metrics 接口定义](../../trpc/metrics/metrics.h)，提供了模调监控上报和属性监控上报能力。

## 插件配置

只要在编译时打开了 Prometheus 相关的[编译选项](#开启 prometheus 监控)，框架就会在启动时自动注册 Prometheus 监控插件。可以框架配置文件中对插件的参数进行配置（不配置则全部使用默认参数）：

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
      auth_cfg:
        iss: admin
        sub: prometheus-pull
        aud: trpc-server
        secret: test
```

配置项说明：

| 参数 | 类型 | 是否必须配置 | 说明 |
| ------ | ------ | ------ | ------ |
| histogram_module_cfg | 序列（Sequences） | 否，默认为 [1, 10, 100, 1000] | 模调监控耗时分布的统计区间，单位为ms |
| const_labels | 映射（Mappings） | 否，默认为空 | 每个RPC统计数据默认附带的标签 |
| auth_cfg | 鉴权（Mappings） | 否，默认为空 | 鉴权相关参数配置 | 

## 模调监控上报

**模调监控是指RPC模块间的调用监控（ModuleReport），包括主调监控（统计客户端调用信息）和被调监控（统计服务端调用信息）。**

### 主调监控上报

只需在框架配置文件的 `client` 中加上 `prometheus` 拦截器，即可开启主调监控：

```yaml
client:
  ...
  filter:
    - prometheus
```

**开启后框架会自动采集主调的 RPC 调用数据。**

统计数据：

| 监控名 | 监控类型 | 说明 |
| ------ | ------ | ------ |
| rpc_client_counter_metric | Counter | 客户端发起的调用总次数 |
| rpc_client_histogram_metric | Histogram | 客户端调用的耗时分布（单位：ms） |

统计标签：

| Key | Value |
| ------ | ------ |
| physicEnv | namespace |
| userEnv | env |
| aApp | 主调app名 |
| aServer | 主调server名 |
| aService | 主调service名 |
| aIp | 主调ip地址 |
| pApp | 被调app名 |
| pServer | 被调server名 |
| pService | 被调service名 |
| pInterface | 被调接口名 |
| pIp | 被调ip地址 |
| pContainer | 被调容器名 |
| pConSetId | 被调所属set |
| frame_ret_code | 调用的框架错误码 |
| interface_ret_code | 调用的接口错误码 |


### 被调监控上报

只需在框架配置文件的 `server` 中加上 `prometheus` 拦截器，即可开启被调监控：

```yaml
server:
  ...
  filter:
    - prometheus
```

**开启后框架会自动采集被调的RPC调用数据。**

统计数据：

| 监控名 | 监控类型 | 说明 |
| ------ | ------ | ------ |
| rpc_server_counter_metric | Counter | 服务端收到的请求总次数 |
| rpc_server_histogram_metric | Histogram | 服务端处理请求的耗时分布（单位：ms） |

统计标签：

| Key | Value |
| ------ | ------ |
| physicEnv | namespace |
| userEnv | env |
| aApp | 主调app名 |
| aServer | 主调server名 |
| aService | 主调service名 |
| aIp | 主调ip地址 |
| pApp | 被调app名 |
| pServer | 被调server名 |
| pService | 被调service名 |
| pInterface | 被调接口名 |
| pIp | 被调ip地址 |
| pContainer | 被调容器名 |
| pConSetId | 被调所属set |
| frame_ret_code | 调用的框架错误码 |
| interface_ret_code | 调用的接口错误码 |


## 属性监控上报

### 属性监控项

除了框架自动采集的RPC调用数据外，插件内部还定义了一组属性监控项，用于用户对其他需要的数据进行采集和统计：

| 监控名 | 类型 |
| ------ | ------ |
| prometheus_counter_metric | Counter |
| prometheus_gauge_metric | Gauge |
| prometheus_summary_metric | Summary |
| prometheus_histogram_metric | Histogram |

### 统计策略

插件提供了如下的统计策略：

| 统计策略 | 对应监控项 | 说明 |
| ------ | ------ | ------ |
| ::trpc::MetricsPolicy::SET | prometheus_gauge_metric | 设置值，监控值的变化 |
| ::trpc::MetricsPolicy::SUM | prometheus_counter_metric | 计算标签的累积计数 |
| ::trpc::MetricsPolicy::MID | prometheus_summary_metric | 统计数据的中位数 |
| ::trpc::MetricsPolicy::QUANTILES | prometheus_summary_metric | 统计数据的具体分位数值 |
| ::trpc::MetricsPolicy::HISTOGRAM | prometheus_histogram_metric | 统计数据的区间分布 |

### 类型说明

属性上报时会用到一些框架定义的特殊数据类型，在这里统一进行说明。

1. `SummaryQuantiles`

    该类型的参数用于指定要统计的分位数，在进行 `QUANTILES` 统计时会用到。实际的类型为：`std::vector<std::vector<double>>`，对应 Prometheus 中的 `Summary::Quantiles`。

    其赋值不能为空，且每个分位数必须由两个值组成，第一个表示要统计的分位数，第二个值表示可以接受的误差范围。例如要统计允许误差为0.05的0.5分位数，和允许误差为0.1的0.9分位数，则可以设置为：`{{0.5, 0.05}, {0.9, 0.1}}`。

2. `HistogramBucket`

    该类型的参数用于指定数据统计的区间，在进行 `HISTOGRAM` 统计时会用到。实际类型为：`std::vector<double>`，对应 Prometheus 中的 `Histogram::BucketBoundaries`。

### 上报方式

详细的使用例子可以参考：[Prometheus example](../../examples/features/prometheus/proxy/)

#### 插件便捷接口上报

Prometheus 监控插件提供了一组适配 Prometheus 数据的便捷上报接口，可以不用像通用的单维属性上报、多维属性上报接口那样繁琐地构造通用数据结构进行上报（**推荐用户使用这些接口**）。

1. API 文件

    要使用这组接口，需要首先引入 tRPC-Cpp 框架的 `Prometheus API` 文件：

    ```cpp
    #include "trpc/metrics/prometheus/prometheus_metrics_api.h"
    ```

2. 上报 `SET` 类型数据

    ```cpp
    namespace trpc::prometheus {
    
    /// @brief Reports metrics data with SET type
    /// @param labels prometheus metrics labels
    /// @param value the value to set
    /// @return report result, where 0 indicates success and non-zero indicates failure
    int ReportSetMetricsInfo(const std::map<std::string, std::string>& labels, double value);
    
    }
    ```

3. 上报 `SUM` 类型数据

    ```cpp
    namespace trpc::prometheus {
    
    /// @brief Reports metrics data with SUM type
    /// @param labels prometheus metrics labels
    /// @param value the value to increment
    /// @return report result, where 0 indicates success and non-zero indicates failure
    int ReportSumMetricsInfo(const std::map<std::string, std::string>& labels, double value);

    }
    ```

4. 上报 `MID` 类型数据

    ```cpp
    namespace trpc::prometheus {

    /// @brief Reports metrics data with MID type
    /// @param labels prometheus metrics labels
    /// @param value the value to observe
    /// @return report result, where 0 indicates success and non-zero indicates failure
    int ReportMidMetricsInfo(const std::map<std::string, std::string>& labels, double value);

    }
    ```

5. 上报 `QUANTILES` 类型数据

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

6. 上报 `HISTOGRAM` 类型数据

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

#### 通用单维属性上报

Prometheus 监控插件支持框架通用的单维属性上报方式，即通过构造 `::trpc::TrpcSingleAttrMetricsInfo` 然后使用`::trpc::metrics::SingleAttrReport` 接口来上报。**Prometheus 的单维属性上报是指上报统计标签只包含一个键值对的数据。**。

设置 `::trpc::TrpcSingleAttrMetricsInfo` 值需要注意：

1. `plugin_name` 必须指定为 Prometheus 监控插件的名字：`::trpc::prometheus::kPrometheusMetricsName`

2. `single_attr_info.policy` 必须正确设置成需要的统计策略。

3. `single_attr_info.name` 表示标签的 key，`single_attr_info.dimension` 表示标签的 value。

4. `single_attr_info.value` 是要进行统计观察的值。

5. `single_attr_info.quantiles`、`single_attr_info.bucket` 分别在进行 `QUANTILES` 和 `HISTOGRAM` 类型数据上报时需要设置。

例如上报 SUM 类型的数据：

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

#### 通用多维属性上报

Prometheus 监控插件支持框架通用的多维属性上报方式，即通过构造 `::trpc::TrpcMultiAttrMetricsInfo` 然后使用`::trpc::metrics::MultiAttrReport`接口来上报。**Prometheus 的多维属性上报是指上报统计标签包含多个键值对的数据。**。

设置 `::trpc::TrpcMultiAttrMetricsInfo` 值需要注意：

1. `plugin_name` 必须指定为 Prometheus 监控插件的名字：`::trpc::prometheus::kPrometheusMetricsName`

2. `multi_attr_info.tags` 设置为对应的键值对集合。

3. `multi_attr_info.values`只能有一个 pair 值，pair.first 设置为需要的统计策略，pair.second 设置为要进行统计观察的值。

4. `multi_attr_info.quantiles`、`multi_attr_info.bucket` 分别在进行 `QUANTILES` 和 `HISTOGRAM` 类型数据上报时需要设置。

例如上报 SUM 类型的数据：

```cpp
#include "trpc/metrics/prometheus/prometheus_metrics_api.h"

::trpc::TrpcMultiAttrMetricsInfo multi_metrics_info;
multi_metrics_info.plugin_name = ::trpc::prometheus::kPrometheusMetricsName;
multi_metrics_info.multi_attr_info.values = {{::trpc::MetricsPolicy::SUM, 1}};
multi_metrics_info.multi_attr_info.tags = {{"multi_test_key1", "multi_test_value1"}, {"multi_test_key2", "multi_test_value2"}};
::trpc::metrics::MultiAttrReport(std::move(multi_metrics_info));
```

# 自定义监控项

用户可以参照 [Prometheus](https://github.com/jupp0r/prometheus-cpp) 原生的 API 自定义自己的监控项和监控数据，而不用局限于框架提供的监控项和监控策略。

框架提供了获取 Prometheus 监控项的接口，用户获取监控族之后即可以上报自己的监控数据。获取接口如下：

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

例如获取一个 `Counter` 类型的监控项并上报：

```cpp
#include "trpc/metrics/prometheus/prometheus_metrics_api.h"

::prometheus::Family<::prometheus::Counter>* counter_family = ::trpc::prometheus::GetCounterFamily(
      "counter_name", "counter_desc", {{"const_counter_key", "const_counter_value"}});
::prometheus::Counter& counter = counter_family->Add({{"counter_key", "counter_value"}});
counter.Increment(1);
```

# 导出监控数据

tRPC-Cpp Prometheus 只负责采集监控数据，没有提供主动 Push 监控数据的功能；但框架提供了对外接口用以直接获取采集到的 Prometheus 数据，另外，若服务开启了 [admin 功能](./admin_service.md)，也可以通过访问 admin 接口来获取监控数据。

## 通过接口获取

框架提供了 `Collect` 接口直接获取原生到的 Prometheus 数据，用户可以自己决定监控数据的使用：

```cpp
namespace trpc::prometheus {

/// @brief Gets monitoring data collected by Prometheus.
std::vector<::prometheus::MetricFamily> Collect();

}  // namespace trpc::prometheus
```

同样地，要使用该接口，需要引用 `trpc/metrics/prometheus/prometheus_metrics_api.h` 文件。

## 通过 admin 获取

如果服务开启了 [admin 功能](./admin_service.md)，则可以通过访问 `http://admin_ip:admin_port/metrics` 获取序列化为字符串后的 Prometheus 数据。

# 鉴权

Prometheus插件鉴权分为两种模式：pull模式 和 push模式，不同模式下的配置方式有所区别。

## pull模式

在pull模式下，使用Json Web Token（JWT）方式来鉴权。需要同时配置**trpc的Prometheus插件**和**Prometheus服务器**。

### 插件配置

插件配置样例如下：

```yaml
plugins:
      auth_cfg:
        iss: admin # issuer 签发人
        sub: prometheus-pull # subject 主题 
        aud: trpc-server # audience 受众
        secret: test # 密钥
```

### Prometheus服务器配置

```yaml
global:
  scrape_interval:     15s
  evaluation_interval: 15s

scrape_configs:
    - job_name: trpc-cpp 
      static_configs:
        - targets: ['127.0.0.1:8889']
          labels:
            instance: trpc-cpp 
      bearer_token: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJwcm9tZXRoZXVzLXB1bGwiLCJhdWQiOiJ0cnBjLXNlcnZlciIsImlzcyI6ImFkbWluIiwiaWF0IjoxNTE2MjM5MDIyfQ.WWYY3jgxelzAXzX0IJSmZUeQeqb5YLV4oBAO7FTUI5o 
```

需要配置**bearer_token**字段，该token可以通过[JWT官方工具](https://jwt.io/)生成。在payload中填写相应的iss，sub和aud字段，verify signature中填写secret字段，加密算法使用默认的 HS256。

## push模式

在push模式下，为了和pushgateway兼容，鉴权使用**username**和**password**的形式。

### 插件配置

插件配置样例如下：

```yaml
plugins:
      auth_cfg:
        username: admin
        password: test
```

### Prometheus服务器配置

```yaml
global:
  scrape_interval:     15s
  evaluation_interval: 15s

scrape_configs:
    - job_name: pushgateway
      static_configs:
        - targets: ['172.17.0.5:9091'] # pushgateway服务器的地址
          labels:
            instance: pushgateway_instance
      basic_auth:
        username: admin
        password: test
```

### Pushgateway服务器配置

需要在Pushgateway服务器启动时，通过带有通过**bcrypt**加密的密文的配置文件启动。Pushgateway启动的配置文件如下：

```yaml
basic_auth_users:                                                                                                                                          
  admin: $2b$12$kXxrZP74Fmjh6Wih0Ignu.uWSiojl5aKj4UnMvHN9s2h/Lc/ui0.S
```

密码的密文可以通过htpasswd工具生成：
```shell
> htpasswd -nbB admin test
admin:$2y$05$5uq4H5p8JyfQm.e16o3xduW6tkI2bTRpArTK4MF4dEuvncpz/bqy.
```


