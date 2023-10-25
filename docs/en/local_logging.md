# Overview
Logs are an important part of software development, and good logs are an important source of information for analyzing the running state of software. The tRPC-Cpp framework provides a complete solution for log printing, collection, and reporting for businesses. The purpose of this article is to provide users with the following information:

- What logging features does the framework provide?
- How does the framework implement these features?
- How to use the standard logging interface?
- How to configure log settings?

Terminology:
- Instance: Equivalent to logger, that is, multiple instances means multiple loggers
- Flow style: Refers to the use of TRPC_FLOW_LOG and TRPC_FLOW_LOG_EX log macros

# Feature Introduction
## Overview of Features

As shown in the figure, the tRPC-Cpp log module is based on SpdLog implementation, which includes two aspects: `log printing programming interface` and `docking with the log output end`. Logger and Sink concepts are introduced in the implementation.
![architecture design](../images/log_design.png)

Logger and Sink are responsible for log printing and docking with the log service system, respectively. They both adopt plug-in programming and can be customized. Users can refer to[how to develop custom log plugins](./custom_logging.md).The definitions of Logger and Sink are as follows:
- Logger is responsible for implementing common interfaces related to log printing. Logger adopts plug-in programming and supports multi-Logger functions. Users can choose their logger to achieve differentiated log printing and collection. It is docked with the log backend through Sink plug-ins.
- Sink, also known as "log output end", is responsible for log collection and reporting functions, including log reporting format and docking with the backend log system, etc. Each Logger can have multiple Sinks, for example, Default, the default logger, can support output to local logs and remote cls at the same time. Sink adopts plug-in programming and can be flexibly expanded.

The tRPC-Cpp framework provides four styles of log printing functions:
- Python style: The most elegant and convenient, recommended.[Detailed syntax](https://fmt.dev/latest/syntax.html)
- Printf style: Printf syntax, type-safe.[Detailed syntax](https://en.wikipedia.org/wiki/Printf_format_string)
- Stream style: Compatible with the old version, the framework prints logs in a more cumbersome way, not recommended for users.
- Flow style: Its underlying layer uses the stream style, but it can support inputting context and specifying logger. It is recommended for scenarios where business logs are printed separately from framework logs.

## Log Printing Interface
First, the framework refers to industry standards to divide the log printing levels from low to high as follows:
| Level | Description |
| :--- | :---: |
| **trace** | The main function of this level of log is to accurately record the running status of every step of specific events in the system|
| **debug** | Indicates that fine-grained information events are very helpful for debugging applications, mainly used for printing some running information during development|
| **info**| Messages at the coarse-grained level emphasize the running process of the application. Print some interesting or important information, which can be used in the production environment to output some important information of the program running, but do not abuse it to avoid printing too many logs|
|**warn** |Indicates potential error situations, some information is not error information, but also needs to give some hints to the programmer|
|**error** |Indicates that although an error event occurs, it does not affect the continued operation of the system. Print error and exception information. If you do not want to output too many logs, you can use this level|
| **critical**|Indicates that every serious error event will cause the application to exit. This level is relatively high. Major errors, at this level you can stop the program directly|

To accommodate different users' habits, the framework provides log macros similar to `python`, `printf`, `stream`, and `flow` for each level of log functions. The examples here are just to give users a rough understanding, and more detailed log macros are introduced below.

- Recommended usage: Python-style log macros, as the performance of fmt::format is about 20% higher than std::ostream.

``` 
// Python style
TRPC_FMT_DEBUG("msg:{}", "test");
// Printf style
TRPC_PRT_DEBUG("msg: %s", "test");
// Stream style
TRPC_LOG_DEBUG("msg:" << "test");
// Flow style
TRPC_FLOW_LOG_EX(ctx, "custom1", "msg:" << "test"); Can specify the input ctx and logger name
```

### 2.2.1 Default Log Macro Interface
- Macro interface description: It uses the default logger and has only one parameter, which represents the log content to be printed. tRPC-Cpp framework logs are based on this.
- Usage scenarios: Framework logs, debug logs
```cpp 
  int i = 0;
  // Python-like
  TRPC_FMT_TRACE("trpc-{}: {}", "cpp");                   // trpc-cpp    
  TRPC_FMT_DEBUG("print floats {:03.2f}", 1.23456);       // "print floats 001.23"
  TRPC_FMT_INFO("hello trpc-{}: {}", "cpp", ++i);         // "hello trpc-cpp: 2"
  TRPC_FMT_ERROR("{} is smaller than {}?", 1, 2);       // "1 is smaller than 2?"
  TRPC_FMT_CRITICAL("coredump msg: {}?", "oom");          // "coredump msg: oom"
 
  // Printf-like
  TRPC_PRT_INFO("just like %s: %d", "printf", ++i);      // "just like printf: 3"
  ...

  // Stream-like
  TRPC_LOG_INFO("stream trace: " << ++i);          // "stream trace: 1"
  ...
```
### Log Macro Interface with Context
- Macro interface description: It has 2 parameters, the first one is context, and the second one is log content. It should be noted that the context here is the base class of ClientContext and ServerContext, so it is applicable on both the client and server sides.

```cpp
TRPC_LOG_TRACE_EX(context, msg);
TRPC_LOG_DEBUG_EX(context, msg);
TRPC_LOG_INFO_EX(context, msg);
TRPC_LOG_WARN_EX(context, msg);
TRPC_LOG_ERROR_EX(context, msg);
TRPC_LOG_DEBUG_EX(context, msg);

// The log level of the framework is fixed at the info level
TRPC_FLOW_LOG_EX(context, instance, msg)
// Example
TRPC_FLOW_LOG_EX(ctx, "custom", "msg：" << "test") // ctx is the context object, custom is the logger name, output: msg: test
```
Explanation:
instance refers to the name of the logger, users can specify different loggers through this macro, and can also get the context information, making the interface more flexible.
The log level of the framework is fixed at the info level, if users want to modify the log level, they can set it based on `TRPC_STREAM`, or they can use it to encapsulate a log macro, here is the implementation of `TRPC_FLOW_LOG`
```
#define TRPC_FLOW_LOG(instance, msg) TRPC_STREAM(instance, trpc::Log::info, nullptr, msg)
#define TRPC_FLOW_LOG_EX(context, instance, msg) \
  TRPC_STREAM(instance, trpc::Log::info, context, msg)
```

### 2.2.3  Log Macro Interface with Specified Logger
- Macro interface description: Can specify logger, customize log output.
- Usage scenarios:
-  Separate business logs from framework logs
-  Different business logs specify different loggers
-  Business logs output to remote

Any of the above scenarios, or any combination of them, can be implemented through the log macro interface with a specified logger.
``` 
  // Flow style macro
  TRPC_FLOW_LOG(instance, msg)
  TRPC_FLOW_LOG_EX(context, instance, msg)
  // Example
  TRPC_FLOW_LOG("custom1", "hello" << "trpc")  // hello trpc
  TRPC_FLOW_LOG_EX("server_context", "custom2", "hello" << "trpc")  // hello trpc

  // fmt style macro interface
  TRPC_LOGGER_FMT_TRACE(instance, format, args...)
  TRPC_LOGGER_FMT_DEBUG(instance, format, args...) 
  TRPC_LOGGER_FMT_INFO(instance, format, args...) 
  TRPC_LOGGER_FMT_WARN(instance, format, args...) 
  TRPC_LOGGER_FMT_ERROR(instance, format, args...) 
  TRPC_LOGGER_FMT_CRITICAL(instance, format, args...)
  // Example 
  TRPC_LOGGER_FMT_WARN("custom3", "i am not {} logger", "default");        // "i am not default logger"
```

### Log Macro Interface with IF Condition
- Macro interface description: Conditional log macro, i.e., when the condition is met, the log is output, otherwise, it is not output.
- Usage scenario 1: Logs are output only if a custom condition is met
``` 
  // fmt style
  TRPC_FMT_TRACE_IF(condition, format, args...)
  TRPC_FMT_DEBUG_IF(condition, format, args...)
  TRPC_FMT_INFO_IF(condition, format, args...)
  TRPC_FMT_WARN_IF(condition, format, args...)
  TRPC_FMT_ERROR_IF(condition, format, args...)
  TRPC_FMT_CRITICAL_IF(condition, format, args...)
  // Example
  TRPC_FMT_INFO_IF(true, "if true, print: {}", "msg");     // "if true, print: msg"
 
  // printf style
  TRPC_PRT_TRACE_IF(condition, format, args...)
  TRPC_PRT_DEBUG_IF(condition, format, args...)
  TRPC_PRT_INFO_IF(condition, format, args...)
  TRPC_PRT_WARN_IF(condition, format, args...)
  TRPC_PRT_ERROR_IF(condition, format, args...)
  TRPC_PRT_CRITICAL_IF(condition, format, args...)
  // Example
  TRPC_PRT_INFO_IF(true, "if true, print: %s",  "msg");   // "if true, print: msg"

  // stream style
  TRPC_LOG_TRACE_IF(condition, msg)
  TRPC_LOG_DEBUG_IF(condition, msg)
  TRPC_LOG_INFO_IF(condition, msg)
  TRPC_LOG_WARN_IF(condition, msg)
  TRPC_LOG_ERROR_IF(condition, msg)
  TRPC_LOG_CRITICAL_IF(condition, msg)
  // Example
  TRPC_LOG_INFO_IF(true, "if true, print: " << "msg");    // "if true, print: msg"

  // All the above macro interfaces can also be used in combination with LOGGER/Context, for example:
  TRPC_LOGGER_FMT_INFO_IF(true, "if true, print: {}", "msg");              // "if true, print: msg"
  TRPC_LOGGER_PRT_INFO_EX(context, true, "if true, print: %s", "msg");     // "if true, print: msg"
  TRPC_LOGGER_LOG_INFO_IF_EX(context, true, "if true, print: " << "msg");  // "if true, print: msg"
```
- Usage scenario 2: Users want to output logs but do not want to output too many logs causing performance degradation.
- In this scenario, you can use conditional macro operators
- TRPC_EVERY_N(c): Set this log to output at most 1 print for every c triggers, the first time must be triggered.
- TRPC_WITHIN_N(ms): Set this log to output at most 1 time within the set ms milliseconds if there are multiple triggers. The first time must be triggered. Usage examples are as follows
``` 
TRPC_FMT_INFO_IF(TRPC_EVERY_N(1000), "condition every 1000");          # Print a log for every 1000 calls
TRPC_FMT_INFO_IF(TRPC_WITHIN_N(1000), "condition within 1000 ms");     # If there is more than one call within 1000ms, print at most one log
```

## Multiple Logger Support
tRPC-Cpp supports multiple loggers at the same time, with each logger having different log levels, print formats, reporting output backends, and businesses being able to set different loggers for different business scenarios. For example:

- Different business modules use different log files for storage
- Different events, based on the degree of attention to events, use different log levels to collect logs

The multi-Logger feature greatly increases the flexibility of log usage.

## Multiple Sink Support
For the same logger, the framework also provides the ability to output logs to multiple log output backends (referred to as "sink") at the same time. Since the sink is also registered to the framework as a plugin, users can choose local, remote (such as cls, smart research logs, etc.), or none. The framework currently supports 4 output endpoints (Sink):
- Local rolling file (LocalFileSink): Configuration name is `local_file`
- Local console output (StdoutSink): Configuration name is stdout.
  The above sink belongs to 2 different types:
- DefaultSink：
    - The above LocalFileSink and StdoutSink are examples.
    - The configuration is written under the sinks field.
    - Requires returning a spdsink.
    - All DefaultSinks belonging to an instance are attached to the built-in spdlogger of DefaultLog, so the received messages are the same.
- RawSink
    - The clsLogSink mentioned above is an example.
    - The configuration is written under the raw_sinks field.
    - Requires implementing a log interface, receiving an additional context parameter, called synchronously by DefaultLog, and ensuring its own thread safety.
    - Its behavior is completely defined by itself. Currently, clsLogSink creates a new spdlogger and spdsink internally and decides how to output based on the context.

It should be emphasized that the log print settings are configured at the Sink level. Users need to configure each output endpoint, such as: the same log needs to be saved in the local log file and output to cls, then the local_file and cls_log sinks must be configured separately.

# Log Configuration
## Configuration Structure
First, let's look at the overall structure of the log configuration. Since the log module implements both logger and sink using a plugin approach, log configurations must be placed in the "plugins" area.
```yaml
plugins:
  log:
    default:
      - name: default # Default logger name
      ...
      sinks:  # DefaultSink type plugins
        local_file:  # Local log sink
          ...
      raw_sinks: # RawSink type plugins
        cls_log: # cls log sink
      - name: custom1  # Custom logger name
        ...
        sinks:
          local_file:
            ...
        raw_sinks:
          cls_log:
```
Explanation:
- The above configuration reflects the multi-logger and multi-sink features. "default" and "custom1" represent the logger names. Each logger configuration is an array of sinks.
- line 4: default represents the framework default logger
- line 6: sinks indicate that the configuration contains DefaultSink type plugins, such as local_file
- line 9: raw_sink indicates that the configuration contains RawtSink type plugins, such as cls_log

## Logger Configuration
Logger configuration is usually inherited directly from the sink it manages, so some fields in the sink can be left unconfigured, such as format, to simplify configuration. First, let's introduce its parameter configuration, designed as follows.
|Configuration item | Type |  Required | Default value |Configuration explanation|
| :--- | :---: | :---: | :---: | :---: |
| name | string  | Optional | Empty | Logger name |
| min_level | int  | Optional | 2 |  Set the minimum log level, logs will only be output if the log level is greater than this |
| format | string  | Optional | Empty | [%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] [%@] %v |
| mode | int  | Optional | 2 |  Optional: 1 Synchronous, 2 Asynchronous, 3 Extreme speed  |

Explanation：
format: Refer to [Format Description](https://github.com/gabime/spdlog/wiki/3.-Custom-formatting)
mode:
- Asynchronous：When printing logs, store the logs in a thread-safe cache queue and return directly. There is a separate thread inside to consume this queue and output to each output endpoint. If the queue is full, the output is blocked; if the queue is empty, the consumption is blocked. Recommended usage.
- Synchronous：When printing logs, directly traverse each output endpoint and wait for completion,
- Extreme speed: Basically the same as asynchronous mode, the only difference is that the output never blocks. Once the queue is full, the oldest log is discarded, and the current log is stored and returned.

``` 
plugins:
  log:
    default:
      - name: default
        min_level: 1 # 0-trace, 1-debug, 2-info, 3-warn, 4-error, 5-critical
        format: "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] [%@] [%!] %v"
        mode: 2 # 1-synchronous, 2-asynchronous, 3-extreme speed
        sinks:
        ...
```

## Sink Configuration
### Use Local Log
#### Local File Log
When the sink is set to "local_file", it means that the log is printed to the local log file. First, let's introduce its configuration items, as follows:
|Configuration item | Type |  Required | Default value | Configuration explanation |
| :--- | :---: | :---: | :---: | :---: |
| format | string  | Optional | Empty |  Log output format, not filled in by default inherits logger |
| eol | bool  | Optional | true |  Whether to wrap when formatting output logs  |
| filename | string  | Required | Empty | Local log file name, default to the current path, can include the path  |
| roll_type | string  | Optional | by_size | Set the log segmentation method, default to by_size, optional by_day-day segmentation, by_hour-hour segmentation |
| reserve_count | int  | Optional | 10 | Rolling log retention file count  |
| roll_size | int  | Optional | 100  x 1024 x 1024 |  Only set this variable for by_size type, which represents the size of a rolling file |
| rotation_hour | int  | Optional | 0 | Only set this variable for by_day type, which represents the cut-off time by day, the specific time is specified by rotation_hour:rotation_minute   |
| rotation_minute | int  | Optional | 0 |   |
| remove_timout_file_switch | bool  | Optional | false | Only set this variable for by_hour type, under the day segmentation mode, the flag for deleting outdated files |
| hour_interval | int  | Optional | 1 | | hour_interval | int  | Optional | 1 | This variable is set for the by_hour type and represents the interval in hours between the hours ||

Local log file output supports three output modes:
- By size: by_size
- By day: by_day
- By hour: by_hour

The following introduces how to configure each
##### Output mode by size
```yaml
plugins:
  log:
    default:
      - name: custom # Can be the default default, can use other logger names
        format: "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] [%@] %v"
        sinks:
          local_file:  # Local log sink
            roll_type: by_size                                  # Output local log mode
            eol: true                                           # Whether to wrap each output automatically. Default wrap
            filename: /usr/local/trpc/bin/log_by_size.log       # Log file name, including path (relative or absolute), default to trpc.log
            roll_size: 10000                                    # Output single file size, default 100M
            reserve_count: 5                                    # Output local log rolling file count, default 9
``` 

##### Output mode by day
```yaml
plugins:
  log:
    default:
      - name: custom # Can be the default default, can use other logger names
        format: "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] [%@] %v"
        sinks:
          local_file:  # Local log sink
            roll_type: by_day                                  # Output local log mode by day
            eol: true                                          # Whether to wrap each output automatically. Default wrap
            filename: /usr/local/trpc/bin/log_by_day.log       # Log file name, including path (relative or absolute), default to trpc.log
            reserve_count: 5                                   # Output local log rolling file count, default 9
            rotation_hour: 0                                   # Represents the cut-off time by day, the specific time is specified by rotation_hour:rotation_minute
            rotation_minute: 0
            remove_timout_file_switch: false                   # Under the day segmentation mode, the flag for deleting outdated files, default not to delete
```

#####  Output mode by hour
```yaml
plugins:
  log:
    default:
      - name: custom # Can be the default default, can use other logger names
        format: "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] [%@] %v"
        sinks:
          local_file:  # Local log sink
            roll_type: by_hour                                   # Output local log mode by hour
            eol: true                                            # Whether to wrap each output automatically. Default wrap
            filename: /usr/local/trpc/bin/log_by_hour.log        # Log file name, including path (relative or absolute), default to trpc.log
            reserve_count: 5                                     # Output local log rolling file count, default 9
            hour_interval: 1                                     # Represents the hour segmentation interval, in hours, default interval is one hour
``` 


#### Local Console Output
When the sink is set to "stdout", it means that the log is printed to the local console output. First, let's introduce its configuration items, as follows:
| Configuration item | Type | Required | Default value | Configuration explanation |
| :--- | :---: | :---: | :---: | :---: |
| format | string | Optional | Empty | Log output format, not filled in by default inherits logger |
| eol | bool | Optional | true | Whether to wrap when formatting output logs |
| stderr_level | int | Optional | 4 | Logs of this level and above are output to standard error output (stderr) |

```yaml
plugins:
  log:
    default:
      - name: custom # Can be the default default, can use other logger names
        format: "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] [%@] %v"
        sinks:
          stdout:  # Local console sink
            eol: true                    # Whether to wrap each output automatically. Default wrap
            stderr_level: 4              # The lowest level output to stderr, default is 4 (ERROR)
``` 

# Classic Scenario Usage Instructions
## Scenario 1: Separating business logs from framework logs, and customizing output for different business logs
Steps:
- Choose macro interface: Choose any of the specified logger log macros to print logs
- Add configuration: Complete logger and sink configuration. Logger configuration can specify format, mode, sink configuration can choose local, remote.

## Scenario 6: How to initialize the log plugin in pure client mode and output the log to a file
Pure client: 
In the pure client scenario, users need to output logs to a file and need to call the interface in their code to complete the initialization of the plugin. There are two ways here:
- Initialize all plugins:
```
trpc::TrpcPlugin::GetInstance()->RegisterPlugins();
```
- Only need to initialize the log plugin
```
trpc::TrpcPlugin::GetInstance()->InitLogging();
```
Note: The server-side framework will help complete the initialization of the log plugin when the service starts, so users don't need to worry about this.

## Scenario 7: How to get the context when printing logs without context being transparently transmitted between business methods?
If no configuration is written or the log module has not been initialized, the logs will be output to the console: level < error will be output to std::cout, otherwise std::cerr. 
To avoid excessive log output (such as trace, debug, etc.), by default, only logs with level >= info will be output. To support different user needs, users can define the TRPC_NOLOG_LEVEL macro to change the default behavior, which needs to be passed to the framework's log module. 
For example, if you need to specify debug, you can try the following two ways:
-  1 Execute bazel test with the following option
   > --copt=-DTRPC_NOLOG_LEVEL=1
-  2 Write in the configuration file .bazelrc, so you don't have to manually specify it every time: 0-trace, 1-debug, 2-info, 3-warn, 4-error, 5-critical
   0-trace, 1-debug, 2-info, 3-warn, 4-error, 5-critical
   > build --copt="-DTRPC_NOLOG_LEVEL=1"

## Scenario 8: If the context is not transparently transmitted between business methods, how to get the context when printing logs?
The framework supports automatically setting the context to the thread's private variables (depending on the business configuration of the thread environment, the fiber environment is set to fiberLocal variables, and the ordinary thread environment is set to threadLocal variables), and provides two methods SetLocalServerContext() and GetLocalServerContext() for business to manually set or use context. The usage example is as follows:
```
#include "trpc/server/server_context.h"
...

::trpc::Status GreeterServiceImpl::SayHello(::trpc::ServerContextPtr context,
                                            const ::trpc::test::helloworld::HelloRequest* request,
                                            ::trpc::test::helloworld::HelloReply* reply) {
  // If you use the future separation mode or are currently in a new thread/fiber manually started by the business, you need to set the context first
  trpc::SetLocalServerContext(context);
  // If you use fiber or future merge mode, the framework has set it up, no need to manually set context, you can use it directly
  
  // Do not pass context and enter other business methods
  ExecBusiness();
  return ::trpc::kSuccStatus;
}

void ExecBusiness() {
  // Use context
  trpc::ServerContextPtr ctx = trpc::GetLocalServerContext();
  TRPC_FMT_INFO("remote address: {}:{}", ctx->GetIp(), ctx->GetPort());
}
```
