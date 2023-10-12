[English](../en/custom_telemetry.md)

# 前言

在tRPC-Cpp中，遥测插件用于采集和上报遥测数据，包括调用链数据（traces）、监控指标数据（metrics）和日志（logs）。是一个集成了链路追踪、监控上报、日志采集三大功能的插件。

这篇文档介绍tRPC-Cpp中遥测插件的接口定义、使用方式，以及开发者如何开发一个遥测插件。

# 插件规范

遥测插件必须遵循[trpc::Telemetry](../../trpc/telemetry/telemetry.h)的接口定义，分别实现链路追踪、监控上报和日志采集功能。

## 链路追踪

遥测插件要求实现 `GetTracing` 接口，用于获取一个调用链插件的实例，即要求链路追踪功能的实现和使用方式需要遵循框架[调用链插件的规范](./custom_tracing.md)。

```cpp
class Telemetry : public Plugin {
 public:
  /// @brief Gets the instance of Tracing plugin which implementing tracing capabilities
  virtual TracingPtr GetTracing() = 0;
};
```

## 监控上报

遥测插件要求实现 `GetMetrics` 接口，用于获取一个监控插件的实例，即要求监控上报功能的实现和使用方式需要遵循框架[监控插件的规范](./custom_metrics.md)。

```cpp
class Telemetry : public Plugin {
 public:
  /// @brief Gets the instance of Metrics plugin which implementing metrics capabilities
  virtual MetricsPtr GetMetrics() = 0;
};
```

## 日志采集

遥测插件要求实现`GetLog`接口，用于获取一个远程日志插件的实例，即要求日志采集功能的实现和使用方式需要遵循框架[远程日志插件的规范](./custom_logging.md)。

```cpp
class Telemetry : public Plugin {
 public:
  /// @brief Gets the instance of Logging plugin which implementing log capabilities
  virtual LoggingPtr GetLog() = 0;
};
```

# 自定义遥测插件

根据遥测插件的规范，可以清楚其功能主要由内部的调用链插件、监控插件、远程日志插件来实现，而插件本身主要负责对内部插件实例进行统一的管理。因此开发者需要：

* 实现一个遥测插件：管理内部的调用链插件、监控插件、远程日志插件实例。
* 实现客户端拦截器/服务端拦截器：管理内部的调用链拦截器、监控拦截器实例。
* 注册遥测插件和对应的拦截器。

具体实现例子可以参考[OpenTelemetry插件](https://github.com/trpc-ecosystem/cpp-telemetry-opentelemetry/)的实现。

## 实现遥测插件

遥测插件需要继承[trpc::Telemetry](../../trpc/telemetry/telemetry.h)。必须重写如下接口：

| 接口 | 说明 | 备注 |
| ------ | ------ | ------ |
| Name | 返回插件名 | 必须保证唯一，不能和其他插件名相同 |
| GetTracing | 获取调用链插件实例 | 用于规范链路追踪功能的实现 |
| GetMetrics | 获取监控插件实例 | 用于规范监控上报功能的实现 |
| GetLog | 获取远程日志插件实例 | 用于规范日志采集功能的实现 |

首先需要实现内部调用链插件、监控插件和远程日志插件，具体请参考[开发自定义调用链插件文档](./custom_tracing.md)、[开发自定义监控插件文档](./custom_metrics.md)和[开发自定义远程日志插件文档](./custom_logging.md)。然后实现遥测插件，对内部的插件实例进行统一管理。遥测插件的主要逻辑如下：

* 负责各插件实例的初始化、启动、停止、销毁等。
* 将远程日志插件实例注册到框架的日志组件中，使得可以通过日志宏来进行日志打印。

注意事项：

* 内部调用链插件实例由遥测插件管理，不能调用`RegisterTracing`注册到调用链插件工厂中。

    调用`MakeClientContext`时，框架也会自动将遥测插件的服务端调用链信息设置给客户端上下文，保证调用链的完整串联。

* 内部监控插件实例由遥测插件管理，不能调用`RegisterMetrics`注册到监控插件工厂中。

    由于没有注册到监控插件工厂，所以无法使用框架[统一的属性上报接口](../../trpc/metrics/trpc_metrics_report.h)进行属性监控上报。**插件开发者应该另外提供一组适配自身监控数据的、方便用户使用的属性上报API接口**，例如[OpenTelemetry插件的监控API](https://github.com/trpc-ecosystem/cpp-telemetry-opentelemetry/blob/master/trpc/telemetry/opentelemetry/metrics/opentelemetry_metrics_api.h)。

## 实现拦截器

首先需要实现内部调用链拦截器和监控拦截器，具体参考[开发自定义调用链插件文档](./custom_tracing.md)和[开发自定义监控插件文档](./custom_metrics.md)。然后实现遥测拦截器（注意`Name`接口返回的名字必须与插件的名字一致），对内部的拦截器实例进行管理。遥测拦截器的主要逻辑如下：

* 其埋点集合包含内部拦截器的全部埋点。
* 当执行到某个埋点时，调用对应的内部拦截器进行逻辑处理。

## 注册插件和拦截器

插件注册的接口：

```cpp
using TelemetryPtr = RefPtr<Telemetry>;

class TrpcPlugin {
 public:
  /// @brief Register custom telemetry plugin
  bool RegisterTelemetry(const TelemetryPtr& telemetry);
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

举个例子进行说明。假设自定义了一个TestTelemetry插件，TestServerFilter、TestClientFilter两个拦截器，那么

1. 对于服务端场景，用户需要在服务启动的`TrpcApp::RegisterPlugins`函数中注册：

    ```cpp
    class HelloworldServer : public ::trpc::TrpcApp {
     public:
      ...
      int RegisterPlugins() override {
        TrpcPlugin::GetInstance()->RegisterTelemetry(MakeRefCounted<TestTelemetry>());
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

      TrpcPlugin::GetInstance()->RegisterTelemetry(MakeRefCounted<TestTelemetry>());
      TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<TestClientFilter>());

      return ::trpc::RunInTrpcRuntime([]() { return Run(); });
    }
    ```

**注意注册的插件和拦截器只需要构造即可，框架启动时会自动调用它们的`Init`函数进行初始化。**
