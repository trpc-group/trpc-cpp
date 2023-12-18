[English](../en/plugin_management.md)

# 前言

tRPC-Cpp 框架基于接口编程的思想，把框架功能抽象成一系列的插件组件，通过插件工厂来管理。框架在运行过程中串联这些插件组件，拼装出完整的功能。我们可以把插件模型分为以下部分：

1. 框架设计层：框架只定义标准接口和使用规范，不限定具体实现，与平台完全解耦。
2. 插件实现层：将插件按框架标准接口封装起来即可实现框架插件。
3. 用户使用层：业务开发只需要注册自己需要的插件即可，按需索取，拿来即用。

在这篇文档中我们对框架插件的管理进行介绍，开发者可以了解到以下内容：

* 框架支持的插件类型
* 插件的注册方式
* 插件的依赖管理

# 插件类型

框架插件化支持的模块类型较多，有框架主体实现相关的，服务治理相关的，还有一些重要的特性。本节对常见插件类型进行介绍。

## 协议插件

支持用户使用自定义的协议进行通信，而不止局限于使用框架默认提供的 `trpc`、`http` 协议。详细介绍请参考[开发自定义协议文档](./custom_protocol.md)。

## 拦截器

框架借鉴 java 面向切面(AOP)的编程思想，支持了拦截器来扩展框架的功能，以及对接不同的生态。例如使用者可以通过自定义拦截器来实现服务限流、服务监控等功能。详细介绍请参考[开发自定义拦截器文档](./filter.md)。

## 服务治理插件

* 名字服务插件
  
    提供了服务注册(registry)、服务发现(selector)、负载均衡(loadbalance)、熔断(circuitbreaker)等能力封装，用于对接各种名字服务系统。详细介绍请参考[开发自定义Naming插件文档](./custom_naming.md)。

* 配置插件

    提供了配置读取相关的接口, 支持读取本地配置文件、远程配置中心配置等，允许插件式扩展支持不同格式的配置文件、不同的配置中心，支持 reload、watch 配置更新。详细介绍请参考[开发自定义配置中心插件文档](./custom_config.md)。

* 日志插件

    提供了通用的日志采集接口, 允许通过插件的方式来扩展日志实现, 允许日志输出到远程。详细介绍请参考[开发自定义Logging插件文档](./custom_logging.md)。

* 监控插件

    提供了监控上报的能力, 支持模调数据的自动采集和上报，以及用户自定义上报单维、多维属性数据。详细介绍请参考[开发自定义Metrics插件文档](./custom_metrics.md)。

* 调用链插件

    提供了分布式跟踪能力，支持在服务调用过程中自动采集并上报调用链数据。详细介绍请参考[开发自定义Tracing插件文档](./custom_tracing.md)。

* 遥测插件

    提供了采集和上报遥测数据的能力，包括调用链数据（traces）、监控指标数据（metrics）和日志（logs）。是一个集成了链路追踪、监控上报、日志采集三大功能的插件。详细介绍请参考[开发自定义Telemetry插件文档](./custom_telemetry.md)。

* 流控和过载保护插件

    提供了流量控制和过载保护的能力。详细介绍请参考
  * [基于并发请求的过载保护插件](./overload_control_concurrency_limiter)。
  * [基于并发 Fiber 个数的过载保护插件](./overload_control_fiber_limiter)。
  * [基于流量控制的过载保护插件](./overload_control_flow_limiter.md)

## 其他特性

* 序列化插件

    支持自定义通信消息的序列化方式，如 protobuf 序列化、json 序列化等。详细介绍请参考[消息序列化文档](./serialization.md)。

* 压缩插件

    支持自定义通信消息的压缩、解压缩方式，如：zlib、gzip等。详细介绍请参考[压缩/解压缩文档](./compression.md)。

# 插件注册方式

用户需要在框架启动时通过框架提供的注册接口，来将插件注册到对应类型的插件工厂中。这些接口由[trpc::TrpcPlugin](../../trpc/common/trpc_plugin.h)提供。

例如注册自定义监控插件：

```cpp
trpc::TrpcPlugin::GetInstance()->RegisterMetrics(trpc::MakeRefCounted<CustomMetrics>());
```

# 依赖管理

不同的插件之间可能存在依赖关系，例如配置的拉取可能需要先查询名字服务。这就要求多个插件的初始化、销毁等操作必须按照一定的顺序来进行。

tPPC-Cpp 框架提供了一套插件依赖管理的方案：

1. 首先插件开发者需要明确指出其实现的插件依赖的其他插件，通过 `GetDependencies` 接口返回依赖的插件的名字。

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

2. 插件使用者注册的插件实例只需要创建，不需要手动调用插件的 `Init`、`Destroy`等接口，它们会由框架自动执行。

3. 框架会分析所有已注册插件之间的依赖关系，并按照以下规则对插件进行管理：

    * 启动时，先调用被依赖插件的 `Init`、`Start`接口，然后再调用不被依赖的插件的 `Init`、`Start`接口。例如插件 A 依赖插件 B，则先初始化插件 B，再初始化插件 A。
    * 退出时，先调用未被依赖的插件的 `Stop`、`Destroy`接口，然后再调用被依赖插件的 `Stop`、`Destroy`接口。例如插件 A 依赖插件B，则先销毁插件 A，再销毁插件 B。
