[中文](../zh/custom_logging.md)

[TOC]

# Overview

The tRPC-Cpp framework's logging module consists of a default log manager (trpc::DefaultLog) and output plugins. The default log manager is responsible for managing multiple log instances and output targets (sinks), and provides some basic logging features such as log level control and log message formatting. On top of this, users can develop remote output plugins. The tRPC-Cpp framework has already provided two local output plugins: stdout (console output) and local_file (file output). It is worth noting that the current version only allows users to develop remote log output plugins; local log output plugins are integrated into the framework and do not support extensions.

This document will guide developers on how to develop custom remote output plugins based on the default log manager.

Now, we will introduce how to develop custom remote output plugins and register them with the default log manager.

## 2. Develop Custom Remote Output Plugin: rawsink

To implement a custom remote output plugin, you need to complete the following steps:

### 2.1 Implement Logging Base Class Interface

First, the plugin developer needs to create a class that inherits from the trpc::Logging interface and implements the following methods in the interface:
| Method Name | Return Type | Description |
|-------------|-------------|-------------|
| std::string LoggerName() const | std::string | Returns the name of the remote logging plugin. |
| void Log(const trpc::Log::Level level, const char* filename_in, int line_in, const char* funcname_in, std::string_view msg, const std::unordered_map<uint32_t, std::any>& extend_fields_msg) | void | Sends log information to the remote server. |以下是一个自定义远程日志插件的示例：

**custom_remote_logging.h**

```cpp
#pragma once

#include <string>
#include "trpc/util/log/logging.h"

namespace trpc::custom_remote_logging {

class CustomRemoteLogging : public trpc::Logging {
 public:
  CustomRemoteLogging(); // Constructor, you can add parameters as needed

  std::string LoggerName() const override;

  void Log(const trpc::Log::Level level, const char* filename_in, int line_in, const char* funcname_in,
           std::string_view msg, const std::unordered_map<uint32_t, std::any>& extend_fields_msg) override;

 private:
  std::string logger_name_;
};

}  // namespace trpc::custom_remote_logging
```

**custom_provider.cc**

```cpp 
#include "custom_remote_logging.h"

namespace trpc::custom_remote_logging {

CustomRemoteLogging::CustomRemoteLogging() : logger_name_("my_remote_logger") {}

std::string CustomRemoteLogging::LoggerName() const {
  return logger_name_;
}

void CustomRemoteLogging::Log(const trpc::Log::Level level, const char* filename_in, int line_in, const char* funcname_in,
                              std::string_view msg, const std::unordered_map<uint32_t, std::any>& extend_fields_msg) {
  // Implement the logic for sending log information to the remote server here
}

}  // namespace trpc::custom_remote_logging
```
In this example, the CustomProvider class inherits from the trpc::config::Provider interface and implements the Name(), Read(), and Watch() methods. Developers can modify this example according to their actual needs to support custom data sources.

### 2.2 Register Custom Remote Logging Plugin

After creating the custom remote logging plugin, you need to register it with the trpc::DefaultLog plugin. You can modify the Init() function to register the custom remote logging plugin with trpc::DefaultLog:

**custom_remote_logging_api.cc**

```cpp
#include "trpc/common/trpc_plugin.h"
#include "custom_remote_logging_api.h"
#include "custom_remote_logging.h"

namespace trpc::custom_remote_logging {

int Init() {
  auto custom_remote_logging = MakeRefCounted<CustomRemoteLogging>();
  // Initialization operations, such as passing configuration parameters
  custom_remote_logging->Init();

  // Get the log factory instance
  auto log_factory = trpc::LogFactory::GetInstance();
  auto default_log = std::dynamic_pointer_cast<trpc::DefaultLog>(log_factory->Get());

  // Register the custom remote logging plugin
  default_log->RegisterRawSink(custom_remote_logging);

  return 0;
}

}  // namespace trpc::custom_remote_logging
```

### 2.2 Register Custom Remote Logging Plugin

After creating the custom remote logging plugin, you need to register it with the trpc::DefaultLog plugin. You can modify the Init() function to register the custom remote logging plugin with trpc::DefaultLog:

**custom_remote_logging_api.cc**

```cpp 
#include "trpc/common/trpc_plugin.h"
#include "custom_remote_logging_api.h"
#include "custom_remote_logging.h"

namespace trpc::custom_remote_logging {

int Init() {
  auto custom_remote_logging = MakeRefCounted<CustomRemoteLogging>();
  // Initialization operations, such as passing configuration parameters
  custom_remote_logging->Init();

  // Get the log factory instance
  auto log_factory = trpc::LogFactory::GetInstance();
  auto default_log = std::dynamic_pointer_cast<trpc::DefaultLog>(log_factory->Get());

  // Register the custom remote logging plugin
  default_log->RegisterRawSink(custom_remote_logging);

  return 0;
}

}  // namespace trpc::custom_remote_logging
```

### 2.3 Configure Custom Remote Logging Output Plugin

In the configuration file, you can add log-related configuration items in the following format:
```yaml
plugins:
  log:
    default:
      - name: default
        min_level: 2
        format: "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] [%@] %v" 
        mode: 2
        sinks:
        raw_sinks:
          custom_remote_logging:
            topic_id: 0***********e
            remote_region: a***********u
            secret_id: A***********t
            secret_key: J***********x
```

| Configuration Item | Type   | Description                     | Example Value                                    |
|--------------------|--------|---------------------------------|---------------------------------------------------|
| name               | String | Logging plugin name             | default                                           |
| min_level          | Integer | Minimum log level               | 2                                                 |
| format             | String | Log format                      | "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] [%@] %v" |
| mode               | Integer | Log mode                        | 2                                                 |
| sinks              | List   | Standard log output list        |                                                   |
| raw_sinks          | Dictionary | Custom RawSink list          |                                                   |
| topic_id           | String | Remote log topic ID             | 0***********e                                     |
| remote_region      | String | Remote log reporting domain     | a***********u                                     |
| secret_id          | String | Cloud service secret_id         | A***********t                                     |
| secret_key         | String | Cloud service secret_key        | J***********x                                     |

### 2.4 Use Custom Remote Log Output Plugin

After completing the development and registration of the custom remote log output plugin, you can use this plugin in your program. Here is a simple example:

```cpp 
#include "custom_remote_logging_api.h"

int main() {
  // Initialize custom remote log plugin
  trpc::custom_remote_logging::Init();

  // Get log factory instance
  auto log_factory = trpc::LogFactory::GetInstance();

  // Use custom remote log plugin to output logs
  log_factory->Get()->LogIt("my_instance", trpc::Log::Level::info, __FILE__, __LINE__, __FUNCTION__,
                            "Hello, custom remote logging!");

  return 0;
}
```
In this example, we first call trpc::custom_remote_logging::Init() to initialize the custom remote log plugin, and then use trpc::LogFactory::GetInstance() to get the log factory instance. Next, we can use the log_factory->Get()->LogIt() method to output logs, which will be sent to the remote server via the custom remote log plugin.


### 3. Print Remote Logs with Log Macros

In the previous sections, we have introduced how to develop a custom remote log plugin and register it in the default log manager. Next, we will introduce how to use macros to print remote logs. In this example, we will use the TRPC_LOGGER_FMT_INFO macro as an example.

## 3.1 Print Remote Logs with Macros

In your code, you can use the TRPC_LOGGER_FMT_INFO macro to print remote logs. For example:

```cpp
#include "custom_remote_logging_api.h"

int main() {
  // Initialize custom remote log plugin
  trpc::custom_remote_logging::Init();

  // Print remote logs using macro
  TRPC_LOGGER_FMT_INFO("my_instance", "Hello, custom remote logging!");

  return 0;
}
```
In this example, we first initialize the custom remote log plugin and then use the TRPC_LOGGER_FMT_INFO macro to print remote logs.

## 3.2 使用其他宏打印远程日志

In addition to the TRPC_LOGGER_FMT_INFO macro, you can also use other macros to print remote logs at different levels. For example:

TRPC_LOGGER_FMT_TRACE: Print remote logs at the trace level. TRPC_LOGGER_FMT_DEBUG: Print remote logs at the debug level. TRPC_LOGGER_FMT_WARN: Print remote logs at the warning level. TRPC_LOGGER_FMT_ERROR: Print remote logs at the error level. TRPC_LOGGER_FMT_CRITICAL: Print remote logs at the critical error level. The usage of these macros is the same as TRPC_LOGGER_FMT_INFO. For example, to print a warning level remote log, you can do this:

```cpp
TRPC_LOGGER_FMT_WARN("my_instance", "This is a warning message.");
```

If you want to check other macros provided by the framework, please refer to trpc/util/log/logging.h.
