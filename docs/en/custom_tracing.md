[中文](../zh/custom_tracing.md)

[TOC]

# Overview

We do not impose any restrictions on the protocol standards followed by the Tracing plugins in tRPC-Cpp. Users can utilize industry-standard protocols such as [OpenTelemetry](https://opentelemetry.io/), [OpenTracing](https://opentracing.io/), or even custom standards. However, we recommend users to use `OpenTelemetry`.

Due to significant differences in the data formats of tracing standards, and the fact that tracing data is typically automatically collected and reported by the framework during RPC calls, independent of business logic, tRPC-Cpp does not abstract the Tracing plugins extensively. Instead, it provides a solution through the framework to automatically collect and report tracing information.

This document introduces the implementation principles of tracing in tRPC-Cpp and guides developers on how to develop a Tracing plugin.

# Principle

## Introduction to common terminology

Before diving into the specific principles, let's first introduce a few common terms in the field of distributed tracing:

`Trace`: A trace represents a chain of all the services and components that a request passes through in a distributed system.

`Span: A span represents an individual operation or unit of work within a trace. A trace typically consists of multiple spans, where each span represents the processing of a request within a service or component.

`ParentSpan`, `ChildSpan`: In a trace, a span can trigger new spans (e.g., initiating a new RPC call). The span that triggers the operation and the span that is triggered are each other's ParentSpan and ChildSpan. Spans are associated with each other through this parent-child relationship.

## Requirements for Information Transmission

To link all the points through which a request passes in a distributed system and form a complete call chain, it is necessary for the contextual information of the call (such as `TraceID`, `Span`, etc.) to be correctly transmitted between services. Therefore, there are certain requirements for the capabilities of communication protocols and RPC frameworks:

1. Protocol

    * The protocol needs to support the transmission of metadata, allowing the tracing data to be passed between different services through metadata. Examples include the transparent information in the `trpc` protocol or the headers in the `http` protocol.

2. RPC framework

    * The client needs to automatically inject the tracing data (such as the `TraceID`, `SpanID`, etc., of the current call) into the metadata and pass it downstream along with the request.
    * The server needs to automatically extract the upstream tracing data from the request's metadata and associate it with the server's own tracing data (for example, inheriting the `TraceID` and setting its `ParentSpanID` as the upstream `SpanID`), thus linking the call chain **from the client to the server**.
    * When the server further calls downstream, it can pass the current server's tracing data to the client context (i.e., using the server's `Span` as the `ParentSpan` of the client's call `Span`), thus linking the call chain **from the server to the client**.

## Solution

The communication protocol just needs to choose a protocol that supports the transmission of metadata. This section we mainly explains how to link the upstream and downstream tracing data in tRPC-Cpp.

### Overall solution

In tRPC-Cpp, we utilize the [Tracing plugin](../../trpc/tracing/tracing.h) for common initialization of tracing data collection and reporting, including configuring sampling policies, backend addresses for data reporting, and reporting methods. The linking and reporting of upstream and downstream tracing data are automatically performed through [filters](filter.md) during the RPC call process. Users only need to configure the corresponding plugin filters in the framework configuration file.

Internally, the tracing data is conventionally stored in the `FilterData` of the `Context`, with the index being the `PluginID` of the plugin. Additionally, a [standardized tracing data format](../../trpc/tracing/tracing_filter_index.h) has been defined for the FilterData of both the server and client.
```cpp
/// @brief The tracing-related data that server saves in the context for transmission
struct ServerTracingSpan {
  std::any span;  // server tracing data, eg. server span in OpenTracing or OpenTelemetry
};

/// @brief The tracing-related data that client saves in the context for transmission
struct ClientTracingSpan {
  std::any parent_span;  // the parent span, used for contextual information inheritance
  std::any span;         // client tracing data, eg. client span in OpenTracing or OpenTelemetry
};
```

### Specific processing flow

![tracing](../../docs/images/tracing.png)

The execution time points and operation descriptions for each position in the graph are as follows:

| Position | Execution time point | Operation |
| ------ | ------ | ------ |
| 1 | The framework automatically executes the pre-filter logic of client side. | 1. As the starting point of this invocation, creating a `Span` with a unique TraceID and no ParentSpan.<br>2. Injecting the current tracing data into the metadata of the request.<br>3. Storing the created Span into the `FilterData` of the ClientContext. |
| 2 | The framework automatically executes the pre-filter logic of server side. | 1. Retrieving the upstream tracing data from the request metadata and creating a `Span` with the same TraceID as the upstream and taking the upstream Span as the ParentSpan.<br>2. Storing the created Span into the `FilterData` of the ServerContext. |
| 3 | Manually constructing ClientContext by the user. | It is necessary to call the framework's `MakeClientContext` interface to construct the ClientContext based on the ServerContext. This interface will automatically set the server's Span as the client's ClientTracingSpan.parent_span. |
| 4 | The framework automatically executes the pre-filter logic of client side. | Except for the difference in the created `Span` (having the same TraceID as ClientTracingSpan.parent_span and taking the ClientTracingSpan.parent_span as ParentSpan), all other operations are the same as in Position 1. |
| 5 | The framework automatically executes the pre-filter logic of server side. | All operations are the same as in Position 2.
| 6 | The framework automatically executes the post-filter logic of server side. | Retrieving the server-side tracing data from the `ServerContext` for reporting. |
| 7 | The framework automatically executes the post-filter logic of client side. | Retrieving the client-side tracing data from the `ClientContext` for reporting. |
| 8 | The framework automatically executes the post-filter logic of server side. | All operations are the same as in Position 6.|
| 9 | The framework automatically executes the post-filter logic of client side. | All operations are the same as in Position 7. |

**Position 1 and Position 2 together accomplish the linkage of the chain from the client to the server, while Position 3 and Position 4 together accomplish the linkage of the chain from the server to the client.**

# Customizing Tracing plugin

According to the analysis of the principle, developers need to:

* Implementing a Tracing plugin: perform common initialization for tracing data collection and reporting.
* Implementing two filters: perform linkage and information reporting during the RPC invocation process.
* Registering the Tracing plugin and its corresponding filters.

You can refer to the implementation of the [Tracing capabilities in the OpenTelemetry plugin](https://github.com/trpc-ecosystem/cpp-telemetry-opentelemetry/tree/master/trpc/telemetry/opentelemetry/tracing) for specific examples.

## Implementing Tracing plugin

The Metrics plugin needs to inherit [trpc::Tracing](../../trpc/tracing/tracing.h). And the interfaces that must be overridden are as follows:

| Interface | Description | Note |
| ------ | ------ | ------ |
| Name | Return the plugin name | It must be unique and cannot be the same as other plugin names. |

We recommend performing common initialization for tracing data collection and reporting in the `Init` function of the plugin. For example, setting up the `Exporter`, `Sampler`, `Processor`, and `Provider` in `OpenTelemetry`.

## Implementing filters

For specific information about filters, please refer to the [filters documentation](filter.md). **It is important to note that the name returned by the filters' `Name` interface must match the name of the plugin.**

### Implementing client filter

#### Filter points selection

The execution of pre-filter logic needs to be done before protocol encoding, allowing the injection of tracing data into the protocol's metadata. Therefore, there are two options for selecting: "`CLIENT_PRE_RPC_INVOKE` + `CLIENT_POST_RPC_INVOKE`" or "`CLIENT_PRE_SEND_MSG` + `CLIENT_POST_RECV_MSG`". The main difference lies in the timing statistics of the calls and whether unserialized user data is required.

#### Pre-filter handling

At the pre-filter point of the filter, the following logic needs to be completed:

1. Creating Span

    First, it is necessary to retrieve the `ClientTracingSpan` from the `FilterData` of `ClientContext`:
    ```cpp
    ClientTracingSpan* client_span = context->GetFilterData<ClientTracingSpan>(PluginID);
    ```

    There may be several possible scenarios:

    * client_span is a null pointer: If ClientContext is not constructed using the `MakeClientContext` interface based on ServerContext, the framework will not automatically set the ClientTracingSpan. In this case, the ClientFilter needs to **create the ClientTracingSpan manually and set it in the FilterData**.
        ```cpp
        context->SetFilterData(PluginID, ClientTracingSpan());
        client_span = context->GetFilterData<ClientTracingSpan>(PluginID);
        ```
    * client_span is not null, but client_span->parent_span is null: `MakeClientContext` automatically sets the ClientTracingSpan, but since the server does not have any tracing data (no ServerFilter configured), the parent_span is null.
    * both client_span and client_span->parent_span are not null: it indicates that there is upstream tracing data that needs to be inherited.

    **If parent_span is null, it is necessary to create a `Span` with a new TraceID and no ParentSpan. If parent_span is not null, it is necessary to create a `Span` with the same TraceID as parent_span and taking parent_span as the ParentSpan.**

2. Injecting the tracing data into the metadata

    **The injection method depends on the communication protocol and the tracing standards.**

    For example, when using the `trpc` protocol and following the `OpenTelemetry` standards, it is necessary to convert Trace, Span, and other information into key-value pairs following the OpenTelemetry standard and write them into the transparent information in the headers of the trpc request.

3. Storing the created Span into the FilterData

    ```cpp
    client_span->span = std::move(Span);
    ```

#### Post-filter handling

At the post-filter point of the filter, the logic to be completed is relatively simple, just need to retrieve the client tracing data from `ClientContext` and report it.

### Implementing server filter

#### Filter points selection

The execution of pre-filter logic needs to be done after protocol decoding, allowing extracting upstream tracing data from the metadata of the protocol. Therefore, there are two options for selecting: "`SERVER_POST_RECV_MSG` + `SERVER_PRE_SEND_MSG`" or "`SERVER_PRE_RPC_INVOKE` + `SERVER_POST_RPC_INVOKE`". The main difference lies in the timing statistics of the calls and whether unserialized user data is required.

#### Pre-filter handling

At the pre-filter point of the filter, the following logic needs to be completed:

1. Extracting the upstream tracing data from the metadata of the request

    **The extraction method depends on the communication protocol and the tracing standards.**

    For example, when using the `trpc` protocol and following the `OpenTelemetry` standards, it is necessary to extract the key-value pairs corresponding to the tracing from the transparent information in the trpc request header, and then reconstruct it into the data structure of OpenTelemetry.

    Note that there may be two possible scenarios when extracting the results:

    * tracing data is empty: it indicates that the upstream is not part of the observed call chain and there is no tracing data to inherit.
    * tracing data is not empty: it indicates that the upstream is part of the observed call chain and there is tracing data to inherit.

2. Creating Span

    **If the extracted upstream tracing data is empty in step 1, it is necessary to create a Span with a new TraceID and no ParentSpan. If the extracted upstream tracing data is not empty, it is necessary to create a Span with the same TraceID as the upstream and taking the upstream Span as ParentSpan.**

3. Storing the created Span into the FilterData

    ```cpp
    ServerTracingSpan svr_span;
    svr_span.span = std::move(Span);
    context->SetFilterData<ServerTracingSpan>(PluginID, std::move(svr_span));
    ```

#### Post-filter handling

At the post-filter point of the filter, the logic to be completed is relatively simple, just need to retrieve the server tracing data from `ServerContext` and report it.

## Registering the plugin and filters

The interface for registering Tracing plugin:
```cpp
using TracingPtr = RefPtr<Tracing>;

class TrpcPlugin {
 public:
  /// @brief Register custom tracing plugin
  bool RegisterTracing(const TracingPtr& tracing);
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

Let's illustrate with an example. Assuming that we implement a TestTracing plugin, along with TestServerFilter and TestClientFilter filters.

1. For server scenarios, users need to register in the `TrpcApp::RegisterPlugins` function during service startup:
    ```cpp
    class HelloworldServer : public ::trpc::TrpcApp {
     public:
      ...
      int RegisterPlugins() override {
        TrpcPlugin::GetInstance()->RegisterTracing(MakeRefCounted<TestTracing>());
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

      TrpcPlugin::GetInstance()->RegisterTracing(MakeRefCounted<TestTracing>());
      TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<TestClientFilter>());

      return ::trpc::RunInTrpcRuntime([]() { return Run(); });
    }
    ```

**Note that the registered plugins and filters only need to be constructed. The framework will automatically invoke their `Init` function during startup for initialization.**
