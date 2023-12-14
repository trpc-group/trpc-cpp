[中文](../zh/plugin_management.md)

# Overview

In the tRPC framework, following the principle of interface programming, framework functionalities are abstracted into a series of plugin components, which are managed through a plugin factory. During runtime, the framework connects these plugin components together to assemble a complete set of functionalities. We can divide the plugin model into the following parts:

1. Framework design layer: The framework only defines standard interfaces and usage specifications, without specifying specific implementations, completely decoupled from the platform.
2. Plugin implementation layer: By encapsulating plugins according to the framework's standard interfaces, framework plugins can be implemented.
3. User usage layer: Business development only needs to register the required plugins, on-demand retrieval, and ready-to-use.

In this document, we will introduce the management of framework plugins. Developers can learn the following topics:

* The supported plugin types in the framework.
* Methods for registering plugins.
* Dependency management for plugins.

# Plugin type

The framework supports a wide range of module types for pluginization, including those related to the core implementation of the framework, service governance, and other important features. This section provides an introduction to common plugin types.

## Protocol

It supports users to communicate using custom protocols, not limited to the default protocols provided by the framework such as `trpc` and `http`. For more details, please refer to the [documentation of customizing thirdparty protocol](./custom_protocol.md).

## Filter

The framework takes inspiration from Java's Aspect-Oriented Programming (AOP) paradigm and supports filters to extend the functionality of the framework and integrate with different ecosystems. For example, users can implement features like service rate limiting and monitoring by customizing filters. For more details, please refer to the [documentation of customizing filter](./fiber.md).

## Service Governance Plugin

* Naming

    It provides encapsulation of capabilities such as service registration (registry), service discovery (selector), load balancing (load balance), circuit breaking (circuit breaker), etc., for integration with various naming service systems. For more details, please refer to the [documentation of customizing Naming plugin](./custom_naming.md).

* Config

    It provides interfaces for reading configuration, supporting reading local configuration files, remote configuration center configurations, etc. It allows plugin-based extension to support different formats of configuration files, different configuration centers, and supports reloading and watching configuration updates. For more details, please refer to the [documentation of customizing Config plugin](./custom_config.md).

* Logging

    It provides a generic logging collection interface, allowing the extension of logging implementation through plugins, and supports logging output to remote destinations. For more details, please refer to the [documentation of customizing Logging plugin](./custom_logging.md).

* Metrics

    It provides the capability of metrics reporting, supporting automatic collection and reporting of inter-module metrics data, as well as user-defined reporting of single-dimensional and multi-dimensional attribute metrics data. For more details, please refer to the [documentation of customizing Metrics plugin](./custom_metrics.md).

* Tracing

    It provides distributed tracing capability, supporting automatic collection and reporting of traces data during service calls. For more details, please refer to the [documentation of customizing Tracing plugin](./custom_tracing.md).

* Telemetry

    It provides the capability of collecting and reporting telemetry data, including traces, metrics, and logs. It is a plugin that integrates three major functionalities: distributed tracing, metrics reporting, and logs collection. For more details, please refer to the [documentation of customizing Telemetry plugin](./custom_telemetry.md).

* Flow-Control & Overload-Protect

    It provides the ability for flow controlling and overload protecting. For more details, please refer to the **documentation of customizing Flow-Control & Overload-Protect plugin**.
  * [Concurrent requests limiter](./overload_control_concurrency_limiter.md)
  * [Concurrent fibers limiter](./overload_control_fiber_limiter.md)
  * [Flow control limiter plugin](./overload_control_flow_limiter.md)

## Other features

* Serialization

    It supports customizing serialization methods for communication messages, such as protobuf serialization, JSON serialization, etc. For more details, please refer to the [Serialization Documentation](./serialization.md).

* Compressor

    It supports customizing compression and decompression methods for communication messages, such as zlib, gzip, etc. For more details, please refer to the [Compression/Decompression Documentation](./compression.md).

# Method for plugin registration

The users need to register the plugins into the corresponding plugin factory using the registration interface provided by the framework during startup. These interfaces are provided by [trpc::TrpcPlugin](../../trpc/common/trpc_plugin.h).

For example, registering a custom metrics plugin:

```cpp
trpc::TrpcPlugin::GetInstance()->RegisterMetrics(trpc::MakeRefCounted<CustomMetrics>());
```

# Dependency management

There may be dependencies between different plugins, for example, configuration retrieval may require querying the name service first. This requires that the initialization, destruction, and other operations of multiple plugins must be performed in a certain order.

tPPC-Cpp framework provides a set of solutions for plugin dependency management.

1. First, plugin developers need to explicitly specify the other plugins that their implementation depends on, and return the names of the dependent plugins through the `GetDependencies` interface.

    ```cpp
    class Plugin : public RefCounted<Plugin> {
    public:
    /// @brief get the collection of plugins that this plugin depends on
    ///        dependent plugins need to be initialized first
    /// @note If there are cases of dependencies with the same name (possible in cases where the types are
    ///       different), the plugin_name needs to follow the following format: 'plugin_name#plugin_type', where
    ///       plugin_name is the name of the plugin and plugin_type is the corresponding index of the plugin type.
    virtual void GetDependencies(std::vector<std::string>& plugin_names) const {}
    };
    ```

2. The plugin instances registered by the plugin users only need to be created, and there is no need to manually invoke the `Init`, `Destroy`, and other interfaces of the plugin. They will be automatically executed by the framework.

3. The framework will analyze the dependency relationships between all registered plugins and manage the plugins according to the following rules.

    * At startup, the `Init` and `Start` interfaces of the dependent plugins are called first, followed by the `Init` and `Start` interfaces of the plugins that are not dependent. For example, if plugin A depends on plugin B, plugin B is initialized first, followed by the initialization of plugin A.
    * At shutdown, the `Stop` and `Destroy` interfaces of the plugins that are not dependent are called first, followed by the `Stop` and `Destroy` interfaces of the dependent plugins. For example, if plugin A depends on plugin B, plugin A is destroyed first, followed by the destruction of plugin B.
