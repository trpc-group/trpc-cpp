[中文](../zh/admin_service.md)

[TOC]

# Overview

The tRPC-Cpp comes with a built-in management service based on the HTTP protocol, providing a set of operational management interfaces for users to view and modify the status of the service. Users can invoke these management commands through a web browser or by constructing HTTP requests manually.

This document introduces the usage of the management service, where developers can learn the following topics:

* The way to enable the management service.
* The basic functionalities provided by the management service:
  * View the version and configuration of the framework.
  * View and modify the log level of the framework.
  * View the server-side statistics such as connection count, request count, latency information, etc.
  * View framework and user-defined tvar variables.
  * Collect usage information of CPU, memory, and other resources for system monitoring and analysis
  * View the rpcz information.
  * Get the prometheus metrics data.
* The way to customize management commands.

# Enable the management service
By default, tRPC-Cpp does not start the management service. To enable it, users need to explicitly configure the `"admin_ip"` and `"admin_port"` of the `"server"` in the framework configuration file, ensuring that admin_port is not set to 0. For example:
```yaml
global:
  ...
server:
  ...
  admin_ip: 0.0.0.0
  admin_port: 8889
```

# Built-in management commands
The framework provides a set of built-in management commands that allow users to conveniently view and modify the service status.

## The way to access
The framework's built-in management commands can be invoked in two ways.

### Access via a web browser

Users can enter `http://admin_ip:admin_port` in a web browser to access the web homepage of the tRPC-Cpp management service. From there, you can click on the corresponding links for each module to perform operations.

![main page](../images/admin_main_page.png)

The functionality of each module on the homepage is described as follows.

| Module | Functionality |
| ------ | ------ |
| config | View the configuration of the framework. |
| cpu | View the CPU usage. |
| heap | View the memory usage. |
| logs | View and modify the log level of the framework. |
| stats | View the server-side statistics such as connection count, request count, latency information, etc. |
| sysvars | View the system resource information, such as CPU configuration, load, IO, and memory usage of current process, etc. |

### Invoke by constructing HTTP requests manually

Users can construct HTTP requests manually to invoke the management commands provided by the framework, for example, by using the curl tool. The list of available management commands for invocation is as follows.

| Command | HTTP Method | Parameters | Functionality |
| ------ | ------ | ------ | ------ |
| /cmds | GET | None | View the list of management commands, including all built-in and user-defined commands. |
| /version | GET | None | View the framework version information. |
| [/cmds/loglevel](#view-log-level) | GET | [logger](#view-log-level) | View the log level. |
| [/cmds/loglevel](#modify-log-level) | PUT | [logger, value](#modify-log-level) | Modify the log level. |
| [/cmds/reload-config](#reload-framework-configuration) | POST | None | Reload framework configuration. |
| [/cmds/stats](#view-the-server-statistics-information) | GET | None | View the server statistics, such as connection count, request count, delay, etc. |
| [/cmds/var](#view-the-framework-and-user-defined-tvar-variables) | GET | None | View the framework and user-defined tvar variables. |
| [/cmds/profile/cpu](#collect-the-cpu-usage-information) | POST | [enable](#collect-the-cpu-usage-information) | Collect the CPU usage information. |
| [/cmds/profile/heap](#collect-the-memory-usage-information) | POST | [enable](#collect-the-memory-usage-information) | Collect the memory usage information. |
| [/cmds/rpcz](#view-the-rpcz-information) | GET | See [rpcz documentation](./rpcz.md) | View the rpcz information. |
| [/metrics](#get-the-prometheus-metrics-data) | GET | None | Get the prometheus metrics data. |
| [/client_detach](#disconnect-from-a-client-address) | POST | [service_name, remote_ip](#disconnect-from-a-client-address) | Disconnect from a client address. |

## Usage

This section introduces the main functionalities of built-in management commands and how to use them. Since almost all operations that can be performed on the management page can also be accessed by constructing HTTP requests, we will only provide detailed instructions on accessing them by constructing HTTP requests (using the curl tool as an example). The corresponding browser usage method is to find the corresponding module on the page and click on the operation.

### View/Modify log level
#### View log level
Corresponding interface: `GET /cmds/loglevel`

Parameters:

| Parameter Name | Type | Description | Required |
| ------ | ------ | ------ | ------ |
| logger | string | The name of the logger to be queried. | No, default value is "default" |

Example:
```shell
# Query the log level of the default logger, the "level" in the returned result is the log level.
$ curl http://admin_ip:admin_port/cmds/loglevel?logger=default
{"errorcode":0,"message":"","level":"INFO"}
# Query a non-existent logger, return an error message.
$ curl http://admin_ip:admin_port/cmds/loglevel?logger=not_exist
{"errorcode":-5,"message":"get level failed, does logger exist?"}
```

#### Modify log level
Corresponding interface: `PUT /cmds/loglevel`

Parameters:

| Parameter Name | Type | Description | Required |
| ------ | ------ | ------ | ------ |
| logger | string | The name of the logger to be queried. | No, default value is "default" |
| value | string | The new log level to be set. The valid values for the log level are: TRACE, DEBUG, INFO, WARNING, ERROR, CRITICAL. | Yes |

Example:
```shell
# Modify the log level of the default logger, the "level" in the returned result is the log level after modified.
$ curl http://admin_ip:admin_port/cmds/loglevel?logger=default -X PUT -d 'value=ERROR'
{"errorcode":0,"message":"","level":"ERROR"}
# Use invalid value for 'value', it will return an error message.
$ curl http://admin_ip:admin_port/cmds/loglevel?logger=default -X PUT -d 'value=ERR'
{"errorcode":-3,"message":"wrong level, please use TRACE,DEBUG,INFO,WARNING,ERROR,CRITICAL"}
# Modify a non-existent logger, it will return an error message
$ curl http://admin_ip:admin_port/cmds/loglevel?logger=not_exist -X PUT -d 'value=ERROR'
{"errorcode":-4,"message":"set level failed, does logger exist?"}
```

### Reload Framework Configuration
Corresponding interface: `POST /cmds/reload-config`

Parameters: None

Interface description: **This interface is only used in scenarios where the user-defined configuration is in the same file as the framework configuration and needs to be dynamically updated. Since the resources inside the framework are fixed at startup, the configuration of the framework itself does not support dynamic updates.**

Usage:

1. Add custom configuration to the framework configuration file, such as adding the "custom" configuration:
    ```yaml
    global:
      ...

    server:
      ...

    custom:
      value: 10
    ```

2. Register the configuration update callback function

    The type of callback function：
    ```cpp
    void(const YAML::Node&)
    ```

    The interface for registering:
    ```cpp
    class TrpcApp {
    public:
      /// @brief Register configuration update callback function
      /// @param name configuration item
      /// @param cb callback
      void RegisterConfigUpdateNotifier(const std::string& name,
                                        const std::function<void(const  YAML::Node&)>& cb);
    };
    ```

    The way to register:
    ```cpp
    class HelloworldServer : public ::trpc::TrpcApp {
    public:
      // Register the callback function during business initialization.
      int Initialize() override {
        RegisterConfigUpdateNotifier("notify", [](const YAML::Node& root) {
          ...
        });
      }
    };
    ```

    The parameter "root" of the callback function is the root node of the entire YAML file after parsing. You can use the "ConfigHelper" tool class provided by the framework to find the node corresponding to the business configuration and obtain the new configuration data. For specific usage examples, please refer to the [admin example](../../examples/features/admin/proxy/).

3. Invoke the command
    ```shell
    $ curl http://admin_ip:admin_port/cmds/reload-config -X POST
    {"errorcode":0,"message":"reload config ok"}
    ```

### View the server statistics information
Corresponding interface: `GET /cmds/stats`

Parameters: None

Interface description: **To enable server statistics, you must set the `"enable_server_stats"` configuration of `"server"` to `true` in the framework configuration file. Additionally, you can configure the statistics interval using `"server_stats_interval"`.**
```yaml
server:
  ...
  admin_ip: 0.0.0.0
  admin_port: 8889
  enable_server_stats: true
  server_stats_interval: 60000     # The interval for metric statistics, which uint is milliseconds. If not configured, the default value is 60 seconds.
```

Example:
```shell
$ curl http://dmin_ip:admin_port/cmds/stats
{"errorcode":0,"message":"","stats":{"conn_count":1,"total_req_count":11,"req_concurrency":1,"now_req_count":3,"last_req_count":4,"total_failed_req_count":0,"now_failed_req_count":0,"last_failed_req_count":0,"total_avg_delay":0.18181818181818183,"now_avg_delay":0.3333333333333333,"last_avg_delay":0.25,"max_delay":1,"last_max_delay":1}}
```

Returned results:

| Field | Meaning |
| ------ | ------ |
| conn_count | Current number of connections |
| total_req_count | Total number of requests (including successful and failed) |
| req_concurrency | Current concurrent number of requests |
| now_req_count | Number of requests in the current cycle |
| last_req_count | Number of requests in the last cycle |
| total_failed_req_count | Total number of failed requests |
| now_failed_req_count | Number of failed requests in the current cycle |
| last_failed_req_count | Number of failed requests in the last cycle |
| total_avg_delay | Total request delay |
| now_avg_delay | Request delay in the current cycle |
| last_avg_delay | Request delay in the last cycle |
| max_delay | Maximum delay in the current cycle |
| last_max_delay | Maximum delay in the last cycle |

### View the framework and user-defined tvar variables
Corresponding interface: `GET /cmds/var`

Parameters: None

Interface description: You can directly access "/cmds/var" to view all tvar variables, including those defined by the framework and user-defined ones. By appending a more specific variable path after the URL, you can access a particular variable. For example, "/cmds/var/trpc" accesses framework internal variables, while "/cmds/var/user" accesses user-defined variables. For more detailed usage of tvar variables, please refer to the [tvar usage documentation](./tvar.md).

Example:
```shell
$ curl http://127.0.0.1:8889/cmds/var
{
  "trpc" : 
  {
    "client" : 
    {
      "trpc.test.helloworld.Greeter" : 
      {
        "backup_request" : 0,
        "backup_request_success" : 0
      }
    }
  },
  "user" : 
  {
    "my_count" : 10
  }
}
```

The statistical variables currently provided by the framework are as follows.

| Variable Name | Meaning |
| ------ | ------ |
| trpc/client/service_name/backup_request | The number of times that backup request is triggered by a certain service client. |
| trpc/client/service_name/backup_request_success | The number of successful backup request attempts made by a certain service client. |

### Collect the CPU and memory usage information

#### The way to enable
**By default, tRPC-Cpp does not allow CPU and memory usage information to be collected through management commands. If you need to collect related information, you need to add the "TRPC_ENABLE_PROFILER" macro definition and link "tcmalloc_and_profiler" when compiling the program.**

Below, we will introduce the ways to enable this feature in Bazel and CMake, respectively.

##### Bazel

1. Use the `"trpc_enable_profiler"` compilation option provided by the framework.

    Using this compilation option will automatically define the "TRPC_ENABLE_PROFILER" macro and link "/usr/lib64/libtcmalloc_and_profiler.so". You should make sure that tcmalloc is installed correctly and that "/usr/lib64/libtcmalloc_and_profiler.so" exists.

    For example, add the compilation option in the .bazelrc file to enable it:
    ```
    # In the .bazelrc file
    build --define trpc_enable_profiler=true
    ```

2. Use the `"trpc_enable_profiler_v2"` compilation option provided by the framework.

    Using this compilation option will automatically define the "TRPC_ENABLE_PROFILER" macro, but users need to manually link "libtcmalloc_and_profiler".

    For example, if the user's "libtcmalloc_and_profiler.so" is located in the "/user-path/lib" directory, you can add the compilation option in the .bazelrc file like this:
    ```
    # In the .bazelrc file
    build --define trpc_enable_profiler_v2=true
    ```
    And Then link the "libtcmalloc_and_profiler" in the BUILD file of the server:
    ```
    cc_binary(
        name = "helloworld_server",
        srcs = ["helloworld_server.cc"],
        linkopts = ["/user-path/lib/libtcmalloc_and_profiler.so"],
        deps = [
            "//trpc/common:trpc_app",
        ],
    )
    ```

3. Define the "TRPC_ENABLE_PROFILER" macro and link "tcmalloc_and_profiler" manually.

    For example, you can add the compilation macro in the .bazelrc file like this:
    ```
    # In the .bazelrc file
    build --copt='-DTRPC_ENABLE_PROFILER'
    ```
    And Then link the "libtcmalloc_and_profiler" in the BUILD file of the server:
    ```
    cc_binary(
        name = "helloworld_server",
        srcs = ["helloworld_server.cc"],
        linkopts = ["/user-path/lib/libtcmalloc_and_profiler.so"],
        deps = [
            "//trpc/common:trpc_app",
        ],
    )
    ```

##### CMake

You need to define "TRPC_ENABLE_PROFILER" and link "tcmalloc_and_profiler" in the CMakeLists.txt file.
```cmake
# define "TRPC_ENABLE_PROFILER"
add_definitions(-DTRPC_ENABLE_PROFILER)

...

# link "tcmalloc_and_profiler"
set(TCMALLOC_LIBRARY "/usr/lib64/libtcmalloc_and_profiler.so")
target_link_libraries(${TARGET_SERVER} ${TCMALLOC_LIBRARY})
```

#### Collect the CPU usage information
Corresponding interface: `POST /cmds/profile/cpu`

Parameters:

| Parameter Name | Type | Description | Required |
| ------ | ------ | ------ | ------ |
| enable | string | Whether to collect CPU usage information. Set to "y" to start sampling, and set to "n" to stop sampling. | Yes |

Usage:

1. Start sampling
    ```shell
    $ curl http://admin_ip:admin_port/cmds/profile/cpu?enable=y -X POST
    {"errorcode":0,"message":"OK"}
    ```

2. Stop sampling
    ```shell
    $ curl http://admin_ip:admin_port/cmds/profile/cpu?enable=n -X POST
    {"errorcode":0,"message":"OK"}
    ```
    After successful termination, a file named "cpu.prof" will be generated in the command execution path.

3. Parse the output file

    The built-in tool pprof of gperftools can be used for parsing:
    ```shell
    $ pprof binary_executable_program ./cpu.prof --pdf > cpu.pdf
    ```

#### Collect the memory usage information
Corresponding interface: `POST /cmds/profile/heap`

Parameters:

| Parameter Name | Type | Description | Required |
| ------ | ------ | ------ | ------ |
| enable | string | Whether to collect memory usage information. Set to "y" to start sampling, and set to "n" to stop sampling. | Yes |

Usage:

1. Start sampling
    ```shell
    $ curl http://admin_ip:admin_port/cmds/profile/heap?enable=y -X POST
    {"errorcode":0,"message":"OK"}
    ```

2. Stop sampling
    ```shell
    $ curl http://admin_ip:admin_port/cmds/profile/heap?enable=n -X POST
    {"errorcode":0,"message":"OK"}
    ```
    After successful termination, a file named "heap.prof" will be generated in the command execution path.

3. Parse the output file

    The built-in tool pprof of gperftools can be used for parsing:
    ```shell
    $ pprof binary_executable_program ./heap.prof --pdf > heap.pdf
    ```

### View the rpcz information
Corresponding interface: `GET /cmds/rpcz`

For instructions on how to use rpcz, please refer to the [rpcz documentation](./rpcz.md).

### Get the prometheus metrics data
Corresponding interface: `GET /metrics`

For instructions on how to use prometheus, please refer to the [Prometheus documentation](./prometheus_metrics.md)。

### Disconnect from a client address
Corresponding interface: `POST /client_detach`

Parameters:

| Parameter Name | Type | Description | Required |
| ------ | ------ | ------ | ------ |
| service_name | string | The target service to disconnect from. | Yes |
| remote_ip | string | The target address to disconnect from, in the format of "ip:port". | Yes |

Interface description: **This interface is currently only effective under the default thread model.**

Example:
```shell
# Disconnect all connections to "ip:port" of the "trpc.app.server.service" service
$ curl http://admin_ip:admin_port/client_detach -X POST -d 'service_name=trpc.app.server.service' -d 'remote_ip=ip:port'
# The service to be operated on does not exist, it will return an error message.
$ curl http://admin_ip:admin_port/client_detach -X POST -d 'service_name=trpc.app.server.not_exist' -d 'remote_ip=ip:port'
{"message":"service is not exist"}
```

# Customize management commands

The tRPC-Cpp allows users to customize and register management commands to perform additional management operations as needed by the user. For specific usage examples, please refer to the [admin example](../../examples/features/admin/proxy/).

Usage:

1. Customize commands

    The users need to inherit `trpc::AdminHandlerBase` to implement the logic for custom management commands. Its definition is as follows:
    ```cpp
    class AdminHandlerBase : public http::HandlerBase {
    public:
      /// @brief Handles commands input by user.
      virtual void CommandHandle(http::HttpRequestPtr req, rapidjson::Value& result,
                                rapidjson::Document::AllocatorType& alloc) = 0;

      /// @brief Serves HTTP requests (implements the "Handle" interface of HTTP handler).
      trpc::Status Handle(const std::string& path, trpc::ServerContextPtr context, http::HttpRequestPtr req,
                          http::HttpResponse* rsp) override;

      /// @brief Gets description of the handler.
      const std::string& Description() const { return description_; }

    protected:
      std::string description_;
    };
    ```

    You can choose to override the `CommandHandle` interface or the `Handle` interface to implement the control logic.

    * CommandHandle: It can conveniently return data in JSON format, suitable for commands accessed through the command line.
    * Handle: It allows flexible control over the format of the returned results, for example, returning data in HTML format.

    Example:
    ```cpp
    #include "trpc/admin/admin_handler.h"

    class MyAdminHandler : public ::trpc::AdminHandlerBase {
    public:
      MyAdminHandler() { description_ = "This is my admin command"; }

      void CommandHandle(::trpc::http::HttpRequestPtr req, ::rapidjson::Value& result,
                        ::rapidjson::Document::AllocatorType& alloc) override {
        // add your processing logic here
        TRPC_LOG_INFO("execute the admin command");

        // set the return result
        result.AddMember("errorcode", 0, alloc);
        result.AddMember("message", "success", alloc);
      }
    };
    ```

2. Register commands

    The interface for registering:
    ```cpp
    class TrpcApp {
    public:
      /// @brief Register custom admin command
      /// @param type operation type
      /// @param url path
      /// @param handler admin handler
      void RegisterCmd(trpc::http::OperationType type, const std::string& url, trpc::AdminHandlerBase* handler);
    };
    ```

    The way to register:
    ```cpp
    class HelloworldServer : public ::trpc::TrpcApp {
    public:
      // Register the commands during business initialization.
      int Initialize() override {
        RegisterCmd(::trpc::http::OperationType::GET, "/myhandler", new MyAdminHandler);
      }
    };
    ```

    In the above example, a management command is registered with a path of "/myhandler" and an access type of "GET".

3. Invoke the commands

    After the service is started, you can trigger the custom management command by accessing `http://admin_ip:admin_port/myhandler`.
    ```shell
    $ curl http://admin_ip:admin_port/myhandler
    {"errorcode":0,"message":"success"}
    ```
