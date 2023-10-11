[中文](../zh/custom_telemetry.md)

# Overview

In tRPC-Cpp, Telemetry plugins are used to collect and report telemetry data, including traces, metrics, and logs. It is a plugin that integrates three major functionalities: distributed tracing, metrics reporting, and logs collection.

This document introduces the interface definition, usage, and development process of Telemetry plugins in tRPC-Cpp.

# Plugin Specification

Telemetry plugins must follow to the interface definition of [trpc::Telemetry](../../trpc/telemetry/telemetry.h) and implement the functionalities of distributed tracing, monitoring reporting, and logs collection respectively.

## Distributed tracing

Telemetry plugins are required to implement the `GetTracing` interface, which is used to obtain an instance of a Tracing plugin. This means that the implementation and usage of the distributed tracing functionality must follow the specifications of the framework's [Tracing plugin](./custom_tracing.md).

```cpp
class Telemetry : public Plugin {
 public:
  /// @brief Gets the instance of Tracing plugin which implementing tracing capabilities
  virtual TracingPtr GetTracing() = 0;
};
```

## Metrics reporting

Telemetry plugins are required to implement the `GetMetrics` interface, which is used to obtain an instance of a Metrics plugin. This means that the implementation and usage of the metrics reporting functionality must follow the specifications of the framework's [Metrics plugin](./custom_metrics.md).

```cpp
class Telemetry : public Plugin {
 public:
  /// @brief Gets the instance of Metrics plugin which implementing metrics capabilities
  virtual MetricsPtr GetMetrics() = 0;
};
```

## Logs collection

Telemetry plugins are required to implement the `GetLog` interface, which is used to obtain an instance of a remote Logging plugin. This means that the implementation and usage of the log collection functionality must follow the specifications of the framework's remote [Logging plugin](./custom_logging.md).

```cpp
class Telemetry : public Plugin {
 public:
  /// @brief Gets the instance of Logging plugin which implementing log capabilities
  virtual LoggingPtr GetLog() = 0;
};
```

# Customize Telemetry plugin

According to the specifications of the Telemetry plugin, it is evident that its functionalities is primarily implemented by internal Tracing plugin, Metrics plugin, and remote Logging plugin. The plugin itself is mainly responsible for the unified management of these internal plugin instances. Therefore, developers need to:

* Implement a Telemetry plugin: manage internal instances of Tracing plugin, Metrics plugin, and remote Logging plugin.
* Implement client filter and server filter: manage internal instances of tracing filters and metrics filters.
* Registering the Telemetry plugin and its corresponding filters.

You can refer to the implementation of the [OpenTelemetry plugin](https://github.com/trpc-ecosystem/cpp-telemetry-opentelemetry/) for specific examples.

## Implement Telemetry plugin

The Telemetry plugin needs to inherit [trpc::Telemetry](../../trpc/telemetry/telemetry.h). And the interfaces that must be overridden are as follows:

| Interface | Description | Note |
| ------ | ------ | ------ |
| Name | Return the plugin name | It must be unique and cannot be the same as other plugin names. |
| GetTracing | Get the instance of Tracing plugin. | Used to standardize the implementation of distributed tracing functionality. |
| GetMetrics | Get the instance of Metrics plugin. | Used to standardize the implementation of metrics reporting functionality. |
| GetLog | Get the instance of remote Logging plugin. | Used to standardize the implementation of logs collection functionality. |

First, you need to implement internal Tracing plugin, Metrics plugin, and remote Logging plugin. Please refer to their respective development documents for more details. They are [Custom Tracing Plugin document](./custom_tracing.md), [Custom Metrics Plugin document](./custom_metrics.md), and [Custom Logging Plugin document](). Then you can implement the Telemetry plugin to manage the internal instances of plugins uniformly. The main logic of Telemetry plugin is as follows:

* Responsible for the initialization, startup, shutdown, and destruction of each plugin instance.
* Registers the remote Logging plugin instance with the framework's logging component, enabling logging printing through logging macros.

Notes:

* The internal instance of Tracing plugin are managed by the Telemetry plugin and should not be registered with the tracing plugin factory using `RegisterTracing`.

    When calling `MakeClientContext`, the framework will automatically set the server-side tracing data of the Telemetry plugin to the ClientContext, ensuring the complete linkage of the call chain.

* The internal instance of Metrics plugin are managed by the Telemetry plugin and should not be registered with the metrics plugin factory using `RegisterMetrics`.

    Since it is not registered with the metrics plugin factory, it is not possible to use the framework's [unified attribute reporting interfaces](../../trpc/metrics/trpc_metrics_report.h) for attribute reporting. **The developers should provide a set of attribute reporting API interfaces that are adapted to their own metrics data and convenient for users to use**, such as the [metrics APIs of the OpenTelemetry plugin](https://github.com/trpc-ecosystem/cpp-telemetry-opentelemetry/blob/master/trpc/telemetry/opentelemetry/metrics/opentelemetry_metrics_api.h).

## Implement filters

First, you need to implement the internal tracing filters and metrics filters. Please refer to their respective development documents for more details. They are [Custom Tracing Plugin document](./custom_tracing.md) and [Custom Metrics Plugin document](./custom_metrics.md). Then you can implement the telemetry filters to manage the instances of internal filters. Note that the name returned by the `Name` interface must match the name of the plugin. The main logic of telemetry filters is as follows:

* Its filters' points set includes all the points of the internal filters.
* When reaching a specific filter point, invoke the corresponding internal filter for logical processing.

# Register the plugin and filters

The interface for registering Telemetry plugin:

```cpp
using TelemetryPtr = RefPtr<Telemetry>;

class TrpcPlugin {
 public:
  /// @brief Register custom telemetry plugin
  bool RegisterTelemetry(const TelemetryPtr& telemetry);
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

Let's illustrate with an example. Assuming that we implement a TestTelemetry plugin, along with TestServerFilter and TestClientFilter filters.

1. For server scenarios, users need to register in the `TrpcApp::RegisterPlugins` function during service startup:

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

2. For pure client scenarios, registration should be done after initializing the framework configuration but before starting other framework modules:

    ```cpp
    int main(int argc, char* argv[]) {
      ParseClientConfig(argc, argv);

      TrpcPlugin::GetInstance()->RegisterTelemetry(MakeRefCounted<TestTelemetry>());
      TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<TestClientFilter>());

      return ::trpc::RunInTrpcRuntime([]() { return Run(); });
    }
    ```

**Note that the registered plugins and filters only need to be constructed. The framework will automatically invoke their `Init` function during startup for initialization.**
