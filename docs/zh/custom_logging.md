
## 1. 前言

tRPC-Cpp框架的日志模块由默认日志管理器（trpc::DefaultLog）和输出插件组成。默认日志管理器负责管理多个日志实例和输出目标（sinks），并提供了一些基本的日志功能，如日志级别控制和日志消息格式化等。在此基础上，用户可以开发远程输出插件。tRPC-Cpp框架已经提供了两种本地输出插件：stdout（控制台输出）和local_file（文件输出）。需要注意的是，现有版本仅允许用户开发远程日志输出插件，本地日志输出插件由框架集成，不支持拓展。

本文档将指导开发者如何在默认日志管理器的基础上开发自定义的远程输出插件。

接下来，我们将介绍如何开发自定义的远程输出插件，并将其注册到默认日志管理器中。

## 2. 开发自定义远程输出插件rawsink

要实现一个自定义的远程输出插件，您需要完成以下步骤：

### 2.1 实现Logging基类接口

首先，插件开发者需要创建一个类，继承自 trpc::Logging 接口，并实现接口中的以下方法：
|方法名称 |返回类型 |描述 |
|:--|
|std::string LoggerName() const |std::string |返回远程日志插件的名称。 |
|void Log(const trpc::Log::Level level, const char* filename_in, int line_in, const char* funcname_in, std::string_view msg, const std::unordered_map<uint32_t, std::any>& extend_fields_msg) |void |将日志信息发送到远程服务器。 |
以下是一个自定义远程日志插件的示例：

**custom_remote_logging.h**

```cpp
#pragma once

#include <string>
#include "trpc/util/log/logging.h"

namespace trpc::custom_remote_logging {

class CustomRemoteLogging : public trpc::Logging {
 public:
  CustomRemoteLogging(); // 构造函数，可以根据需要添加参数

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
  // 在这里实现将日志信息发送到远程服务器的逻辑
}

}  // namespace trpc::custom_remote_logging
```
在这个示例中， `CustomProvider` 类继承自 `trpc::config::Provider` 接口，并实现了 `Name()` 、 `Read()` 和 `Watch()` 方法。开发者可以根据实际需求修改这个示例，以实现对自定义数据源的支持。

### 2.2 注册自定义远程日志插件

在创建完自定义的远程日志插件后，您需要将其注册到trpc::DefaultLog插件中。可以修改Init()函数，将自定义的远程日志插件注册到trpc::DefaultLog中：

**custom_remote_logging_api.cc**

```cpp
#include "trpc/common/trpc_plugin.h"
#include "custom_remote_logging_api.h"
#include "custom_remote_logging.h"

namespace trpc::custom_remote_logging {

int Init() {
  auto custom_remote_logging = MakeRefCounted<CustomRemoteLogging>();
  // 初始化操作，如传递配置参数等
  custom_remote_logging->Init();

  // 获取日志工厂实例
  auto log_factory = trpc::LogFactory::GetInstance();
  auto default_log = std::dynamic_pointer_cast<trpc::DefaultLog>(log_factory->Get());

  // 注册自定义远程日志插件
  default_log->RegisterRawSink(custom_remote_logging);

  return 0;
}

}  // namespace trpc::custom_remote_logging
```

### 2.2 注册自定义远程日志插件

在创建完自定义的远程日志插件后，您需要将其注册到trpc::DefaultLog插件中。可以修改Init()函数，将自定义的远程日志插件注册到trpc::DefaultLog中：

**custom_remote_logging_api.cc**

```cpp 
#include "trpc/common/trpc_plugin.h"
#include "custom_remote_logging_api.h"
#include "custom_remote_logging.h"

namespace trpc::custom_remote_logging {

int Init() {
  auto custom_remote_logging = MakeRefCounted<CustomRemoteLogging>();
  // 初始化操作，如传递配置参数等
  custom_remote_logging->Init();

  // 获取日志工厂实例
  auto log_factory = trpc::LogFactory::GetInstance();
  auto default_log = std::dynamic_pointer_cast<trpc::DefaultLog>(log_factory->Get());

  // 注册自定义远程日志插件
  default_log->RegisterRawSink(custom_remote_logging);

  return 0;
}

}  // namespace trpc::custom_remote_logging
```

### 2.3 配置自定义远程日志输出插件

在配置文件中，您可以按照以下格式添加日志相关配置项：

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

| 配置项       | 类型   | 描述                       | 示例值                |
|--------------|--------|----------------------------|-----------------------|
| name         | 字符串 | 日志插件名称               | default               |
| min_level    | 整数   | 最小日志级别               | 2                     |
| format       | 字符串 | 日志格式                   | "[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%l] [%@] %v" |
| mode         | 整数   | 日志模式                   | 2                     |
| sinks        | 列表   | 标准日志输出列表           |                       |
| raw_sinks    | 字典   | 自定义 RawSink 列表        |                       |
| topic_id     | 字符串 | 远程日志主题 ID            | 0***********e         |
| remote_region| 字符串 | 远程日志上报域名           | a***********u         |
| secret_id    | 字符串 | 云服务 secret_id           | A***********t         |
| secret_key   | 字符串 | 云服务 secret_key          | J***********x         |

### 2.4 使用自定义远程日志输出插件

完成自定义远程日志插件的开发和注册后，您可以在程序中使用此插件。以下是一个简单的示例：
```cpp 
#include "custom_remote_logging_api.h"

int main() {
  // 初始化自定义远程日志插件
  trpc::custom_remote_logging::Init();

  // 获取日志工厂实例
  auto log_factory = trpc::LogFactory::GetInstance();

  // 使用自定义远程日志插件输出日志
  log_factory->Get()->LogIt("my_instance", trpc::Log::Level::info, __FILE__, __LINE__, __FUNCTION__,
                            "Hello, custom remote logging!");

  return 0;
}
```
在这个示例中，我们首先调用trpc::custom_remote_logging::Init()初始化自定义远程日志插件，然后使用trpc::LogFactory::GetInstance()获取日志工厂实例。接下来，我们可以使用log_factory->Get()->LogIt()方法输出日志，这些日志将通过自定义远程日志插件发送到远程服务器。


### 3. 配合日志宏打印远程日志

在前面的章节中，我们已经介绍了如何开发自定义的远程日志插件并将其注册到默认日志管理器中。接下来，我们将介绍如何使用宏来打印远程日志。在本示例中，我们将使用TRPC_LOGGER_FMT_INFO宏作为示例。

## 3.1 使用宏打印远程日志

在您的代码中，您可以使用TRPC_LOGGER_FMT_INFO宏来打印远程日志。例如：
```cpp
#include "custom_remote_logging_api.h"

int main() {
  // 初始化自定义远程日志插件
  trpc::custom_remote_logging::Init();

  // 使用宏打印远程日志
  TRPC_LOGGER_FMT_INFO("my_instance", "Hello, custom remote logging!");

  return 0;
}
```
在这个示例中，我们首先调用trpc::custom_remote_logging::Init()初始化自定义远程日志插件。然后，我们使用TRPC_LOGGER_FMT_INFO宏，并传入"my_instance"作为instance名称来打印远程日志。这些日志将通过自定义远程日志插件发送到远程服务器。

## 3.2 使用其他宏打印远程日志

除了TRPC_LOGGER_FMT_INFO宏之外，您还可以使用其他宏来打印不同级别的远程日志。例如：

TRPC_LOGGER_FMT_TRACE：打印追踪级别的远程日志。
TRPC_LOGGER_FMT_DEBUG：打印调试级别的远程日志。
TRPC_LOGGER_FMT_WARN：打印警告级别的远程日志。
TRPC_LOGGER_FMT_ERROR：打印错误级别的远程日志。
TRPC_LOGGER_FMT_CRITICAL：打印严重错误级别的远程日志。
这些宏的使用方法与TRPC_LOGGER_FMT_INFO相同。例如，要打印一个警告级别的远程日志，您可以这样做：
```cpp
TRPC_LOGGER_FMT_WARN("my_instance", "This is a warning message.");
```
如果想查阅框架提供的其它宏，请关注trpc/util/log/logging.h
