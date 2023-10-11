[English](../en/custom_naming.md)
## 1. 前言

tRPC-Cpp框架的名字服务（naming）模块提供了服务发现、服务注册/心跳上报以及限流相关功能，包括selector、registry和limiter插件。用户可以基于tRPC客户端服务代理（结合具体的框架配置项或服务代理配置项）或通过名字服务模块对外API接口（位于trpc/naming/trpc_naming.h）来使用naming模块的功能。在此基础上，用户可以开发自定义的名字服务插件。

在本文档中，我们将指导开发者如何开发自定义的名字服务插件。

## 2. 开发自定义名字服务插件

要实现一个自定义的名字服务插件，您需要完成以下步骤：

### 2.1 实现名字服务基类接口

首先，插件开发者需要创建一个类，继承自相应的名字服务基类（如trpc::Limiter、trpc::Registry或trpc::Selector），并实现基类中的方法。接下来，我们分别为自定义限流器插件（CustomLimiter）、自定义服务注册插件（CustomRegistry）和自定义服务选择器插件（CustomSelector）提供了示例。

自定义限流插件示例：
**custom_limiter.h**

```cpp
#pragma once

#include <string>
#include "trpc/naming/limiter.h"

namespace trpc::custom_limiter {

class CustomLimiter : public trpc::Limiter {
 public:
  CustomLimiter(); // 构造函数，可以根据需要添加参数

  LimitRetCode ShouldLimit(const LimitInfo* info) override;

  int FinishLimit(const LimitResult* result) override;

 private:
  // 添加自定义成员变量
};

}  // namespace trpc::custom_limiter
```

**custom_limiter.cc**

```cpp 
#include "custom_limiter.h"

namespace trpc::custom_limiter {

CustomLimiter::CustomLimiter() {
  // 初始化操作，如传递配置参数等
}

LimitRetCode CustomLimiter::ShouldLimit(const LimitInfo* info) {
  // 在这里实现限流逻辑
}

int CustomLimiter::FinishLimit(const LimitResult* result) {
  // 在这里实现限流完成后的操作，如上报限流数据等
}

}  // namespace trpc::custom_limiter
```
在这个示例中，CustomLimiter 类继承自 trpc::Limiter 基类，并实现了 ShouldLimit() 和 FinishLimit() 方法。开发者可以根据实际需求修改这个示例，以实现对自定义限流器插件的支持。

自定义服务注册插件示例：
**custom_registry.h**

```cpp
#pragma once

#include <string>
#include "trpc/naming/registry.h"

namespace trpc::custom_registry {

class CustomRegistry : public trpc::Registry {
 public:
  CustomRegistry(); // 构造函数，可以根据需要添加参数

  int Register(const TrpcRegistryInfo& info) override;

  int Unregister(const TrpcRegistryInfo& info) override;

 private:
  // 添加自定义成员变量
};

}  // namespace trpc::custom_registry
```

**custom_registry.cc**

```cpp
#include "custom_registry.h"

namespace trpc::custom_registry {

CustomRegistry::CustomRegistry() {
  // 初始化操作，如传递配置参数等
}

int CustomRegistry::Register(const TrpcRegistryInfo& info) {
  // 在这里实现服务注册逻辑
}

int CustomRegistry::Unregister(const TrpcRegistryInfo& info) {
  // 在这里实现服务注销逻辑
}

}  // namespace trpc::custom_registry
```
在这个示例中，CustomRegistry 类继承自 `trpc::Registry 基类，并实现了 Register() 和 Unregister() 方法。开发者可以根据实际需求修改这个示例，以实现对自定义服务注册插件的支持。


自定义服务选择器插件示例：
**custom_selector.h**

```cpp
#pragma once

#include <string>
#include "trpc/naming/selector.h"

namespace trpc::custom_selector {

class CustomSelector : public trpc::Selector {
 public:
  CustomSelector(); // 构造函数，可以根据需要添加参数

  int Select(const TrpcSelectorInfo& info, TrpcEndpointInfo& endpoint) override;

 private:
  // 添加自定义成员变量
};

}  // namespace trpc::custom_selector
```

**custom_selector.cc**

```cpp
#include "custom_selector.h"

namespace trpc::custom_selector {

CustomSelector::CustomSelector() {
  // 初始化操作，如传递配置参数等
}

int CustomSelector::Select(const TrpcSelectorInfo& info, TrpcEndpointInfo& endpoint) {
 
```
在这个示例中，CustomSelector 类继承自 trpc::Selector 基类，并实现了 Select() 方法。开发者可以根据实际需求修改这个示例，以实现对自定义服务选择器插件的支持。

另外，如果用户需要依赖框架自动触发限流及路由选择功能的话，还需要依据[自定义拦截器](./filter.md)文档，实现对应的Limiter和Selector插件，从而将限流和路由选择功能串联到整个框架流程中。

### 2.2 注册自定义名字服务插件

在创建完自定义的名字服务插件后，您需要将其注册到相应的名字服务工厂。

1. 对于服务端场景，用户需要在服务启动的`TrpcApp::RegisterPlugins`函数中注册：
```cpp
class HelloworldServer : public ::trpc::TrpcApp {
 public:
  ...
  int RegisterPlugins() override {
    // 注册Limiter插件及其对应的server/client filter
    TrpcPlugin::GetInstance()->RegisterLimiter(MakeRefCounted<CustomLimiter>());
    TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<CustomLimiterClientFilter>());
    TrpcPlugin::GetInstance()->RegisterServerFilter(std::make_shared<CustomLimiterServerFilter>());

    // 注册Registry插件
    TrpcPlugin::GetInstance()->RegisterRegistry(MakeRefCounted<CustomSelector>());

    // 注册Selector插件及其对应的filter
    TrpcPlugin::GetInstance()->RegisterSelector(MakeRefCounted<CustomSelector>());
    TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<CustomSelectorFilter>());

    return 0;
  }
};
```

2. 对于纯客户端场景，需要在启动框架配置初始化后，框架其他模块启动前注册：
```cpp
int main(int argc, char* argv[]) {
  ParseClientConfig(argc, argv);

  // 注册Limiter插件及其对应的client filter
  TrpcPlugin::GetInstance()->RegisterLimiter(MakeRefCounted<CustomLimiter>());
  TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<CustomLimiterClientFilter>());

  // 注册Selector插件及其对应的filter
  TrpcPlugin::GetInstance()->RegisterSelector(MakeRefCounted<CustomSelector>());
  TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<CustomSelectorFilter>());

  return ::trpc::RunInTrpcRuntime([]() { return Run(); });
}
```

**注意注册的插件和拦截器只需要构造即可，框架启动时会自动调用它们的`Init`函数进行初始化。**

## 3. 使用trpc_naming.h接口

完成自定义名字服务插件的开发和注册后，您可以在程序中使用trpc_naming.h提供的接口来完成服务注册、服务发现和限流操作。

首先，假设您已经完成了自定义服务注册插件（CustomRegistry）、服务选择器插件（CustomSelector）和限流器插件（CustomLimiter）的开发和注册。接下来，您需要在配置文件中添加这些插件的配置。

以下是一个配置文件示例：
```yaml
plugins: # 插件配置
  registry: # 服务注册配置
    custom_registry: # 使用自定义服务注册插件
  selector: # 路由选择配置
    custom_selector: # 使用自定义服务选择器插件
  limiter:  # 访问流量控制配置
    custom_limiter: # 使用自定义限流器插件
```
在这个示例中，我们将自定义插件的名称（custom_registry、custom_selector、custom_limiter）添加到了相应的配置项中。框架会根据配置文件解析并使用相应的插件。

需要注意的是，具体的配置解析过程由框架完成，您只需要确保配置文件中的插件名称与您开发的自定义插件名称相匹配即可。

## 4. 使用自定义名字服务插件

在完成自定义名字服务插件的开发、注册和配置后，您可以在程序中使用这些插件来完成服务注册、服务发现和限流操作。以下是使用trpc_naming.h提供的接口进行操作的示例：

### 4.1 服务注册

```cpp
#include "trpc/naming/trpc_naming.h"

void RegisterService() {
  trpc::TrpcRegistryInfo registry_info;
  registry_info.service_name = "my_service";
  registry_info.endpoint.ip = "127.0.0.1";
  registry_info.endpoint.port = 8080;
    
  int ret = trpc::naming::Register(registry_info);
  if (ret != 0) {
    // 处理注册失败的情况
  }
}
```

### 4.2 服务发现

```cpp
#include "trpc/naming/trpc_naming.h"

void DiscoverService() {
  trpc::TrpcSelectorInfo selector_info;
  selector_info.service_name = "my_service";

  trpc::TrpcEndpointInfo endpoint;
  int ret = trpc::naming::Select(selector_info, endpoint);
  if (ret != 0) {
    // 处理服务发现失败的情况
  } else {
    // 使用找到的服务端点进行通信
  }
}
```

### 4.3 限流操作

```
#include "trpc/naming/trpc_naming.h"

void PerformLimiter() {
  trpc::LimitInfo limit_info;
  limit_info.service_name = "my_service";
  limit_info.caller_ip = "127.0.0.1";
  limit_info.caller_port = 12345;

  trpc::LimitRetCode ret_code = trpc::naming::ShouldLimit(&limit_info);
  if (ret_code != trpc::kLimitRetCodeAllow) {
    // 处理限流情况
  } else {
    // 未触发限流，继续执行后续操作
  }
}
```
以上示例展示了如何使用trpc_naming.h提供的接口来完成服务注册、服务发现和限流操作。请注意，这些示例假设您已经完成了自定义名字服务插件的开发、注册和配置。在实际使用中，您需要根据实际需求和自定义插件的功能调整这些示例。

## 5.小结

本文档介绍了如何开发自定义名字服务插件（包括限流器插件、服务注册插件和服务选择器插件），并展示了如何使用trpc_naming.h提供的接口来完成服务注册、服务发现和限流操作。

开发自定义名字服务插件的关键步骤如下：

1. 创建一个类，继承自相应的名字服务基类（如trpc::Limiter、trpc::Registry或trpc::Selector），并实现基类中的方法。并实现相应的filter。
2. 将自定义插件及filter注册到框架中。
3. 在配置文件中添加自定义插件的配置。

在完成这些步骤后，您可以在程序中使用trpc_naming.h提供的接口来完成服务注册、服务发现和限流操作。
