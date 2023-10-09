[中文](../zh/custom_naming.md)

[TOC]
## 1. Overview

The tRPC-Cpp framework's naming module provides service discovery, service registration/heartbeat reporting, and traffic limiting related functions, including selector, registry, and limiter plugins. Users can use the naming module's functions based on the tRPC client service proxy (combined with specific framework configuration items or service proxy configuration items) or through the external API interface of the naming module (located in trpc/naming/trpc_naming.h). On this basis, users can develop custom naming service plugins.

In this document, we will guide developers on how to develop custom naming service plugins.

## 2. Develop custom naming service plugins

To implement a custom naming service plugin, you need to complete the following steps:

### 2.1 Implement the naming service base class interface

First, the plugin developer needs to create a class, inherit from the corresponding naming service base class (such as trpc::Limiter, trpc::Registry, or trpc::Selector), and implement the methods in the base class. Next, we provide examples for custom limiter plugins (CustomLimiter), custom service registration plugins (CustomRegistry), and custom service selector plugins (CustomSelector).

Custom limiter plugin example:

**custom_limiter.h**

```cpp
#pragma once

#include <string>
#include "trpc/naming/limiter.h"

namespace trpc::custom_limiter {

class CustomLimiter : public trpc::Limiter {
 public:
  CustomLimiter(); // Constructor, can add parameters as needed

  LimitRetCode ShouldLimit(const LimitInfo* info) override;

  int FinishLimit(const LimitResult* result) override;

 private:
  // Add custom member variables
};

}  // namespace trpc::custom_limiter
```

**custom_limiter.cc**

```cpp 
#include "custom_limiter.h"

namespace trpc::custom_limiter {

CustomLimiter::CustomLimiter() {
  // Initialization operations, such as passing configuration parameters, etc.
}

LimitRetCode CustomLimiter::ShouldLimit(const LimitInfo* info) {
  // Implement traffic limiting logic here
}

int CustomLimiter::FinishLimit(const LimitResult* result) {
  // Implement operations after traffic limiting is completed, such as reporting traffic limiting data, etc.
}

}  // namespace trpc::custom_limiter
```
In this example, the CustomLimiter class inherits from the trpc::Limiter base class and implements the ShouldLimit() and FinishLimit() methods. Developers can modify this example according to actual needs to support custom limiter plugins.


Custom service registration plugin example:
**custom_registry.h**

```cpp
#pragma once

#include <string>
#include "trpc/naming/registry.h"

namespace trpc::custom_registry {

class CustomRegistry : public trpc::Registry {
 public:
  CustomRegistry(); // Constructor, can add parameters as needed

  int Register(const TrpcRegistryInfo& info) override;

  int Unregister(const TrpcRegistryInfo& info) override;

 private:
  // Add custom member variables
};

}  // namespace trpc::custom_registry
```

**custom_registry.cc**

```cpp
#include "custom_registry.h"

namespace trpc::custom_registry {

CustomRegistry::CustomRegistry() {
  // Initialization operations, such as passing configuration parameters, etc.
}

int CustomRegistry::Register(const TrpcRegistryInfo& info) {
  // Implement service registration logic here
}

int CustomRegistry::Unregister(const TrpcRegistryInfo& info) {
  // Implement service deregistration logic here
}

}  // namespace trpc::custom_registry
```
In this example, the CustomRegistry class inherits from the `trpc::Registry base class and implements the Register() and Unregister() methods. Developers can modify this example according to actual needs to support custom service registration plugins.


Custom service selector plugin example:
**custom_selector.h**

```cpp
#pragma once

#include <string>
#include "trpc/naming/selector.h"

namespace trpc::custom_selector {

class CustomSelector : public trpc::Selector {
 public:
  CustomSelector(); // Constructor, can add parameters as needed

  int Select(const TrpcSelectorInfo& info, TrpcEndpointInfo& endpoint) override;

 private:
  // Add custom member variables
};

}  // namespace trpc::custom_selector
```

**custom_selector.cc**

```cpp
#include "custom_selector.h"

namespace trpc::custom_selector {

CustomSelector::CustomSelector() {
  // Initialization operations, such as passing configuration parameters, etc.
}

int CustomSelector::Select(const TrpcSelectorInfo& info, TrpcEndpointInfo& endpoint) {
 
```
In this example, the CustomSelector class inherits from the trpc::Selector base class and implements the Select() method. Developers can modify this example according to actual needs to support custom service selector plugins.

### 2.2 Register custom naming service plugins

After creating the custom naming service plugins, you need to register them with the corresponding naming service factory (such as trpc::LimiterFactory, trpc::RegistryFactory, or trpc::SelectorFactory). The following is an example of registering a custom limiter plugin with trpc::LimiterFactory:
**custom_limiter_api.cc**

```cpp
#include "trpc/naming/limiter_factory.h"
#include "custom_limiter_api.h"
#include "custom_limiter.h"

namespace trpc::custom_limiter {

int Init() {
  auto custom_limiter = MakeRefCounted<CustomLimiter>();
  // Initialization operations, such as passing configuration parameters, etc.
  custom_limiter->Init();

  // Get the limiter factory instance
  auto limiter_factory = trpc::LimiterFactory::GetInstance();

  // Register the custom limiter plugin
  limiter_factory->Register(custom_limiter);

  return 0;
}

}  // namespace trpc::custom_limiter
```
Similarly, you can register custom service registration plugins and service selector plugins with the corresponding naming service factories.

## 3. Use the trpc_naming.h interface

After completing the development and registration of custom naming service plugins, you can use the trpc_naming.h interface in the program to complete service registration, service discovery, and traffic limiting operations.

First, assume that you have completed the development and registration of custom service registration plugins (CustomRegistry), service selector plugins (CustomSelector), and limiter plugins (CustomLimiter). Next, you need to add the configuration of these plugins to the configuration file.

Here is an example configuration file:
```yaml
plugins: # Plugin configuration
  registry: # Service registration configuration
    custom_registry: # Use custom service registration plugin
  selector: # Routing selection configuration
    custom_selector: # Use custom service selector plugin
  limiter:  # Access traffic control configuration
    custom_limiter: # Use custom limiter plugin
```
In this example, we added the names of custom plugins (custom_registry, custom_selector, custom_limiter) to the corresponding configuration items. The framework will parse the configuration file and use the corresponding plugins.

Please note that the specific configuration parsing process is completed by the framework, and you only need to ensure that the plugin names in the configuration file match the names of the custom plugins you developed.

## 4. Use custom naming service plugins

After completing the development, registration, and configuration of custom naming service plugins, you can use these plugins in the program to complete service registration, service discovery, and traffic limiting operations. The following are examples of using the trpc_naming.h interface for operations:

### 4.1 Service registration

```cpp
#include "trpc/naming/trpc_naming.h"

void RegisterService() {
  trpc::TrpcRegistryInfo registry_info;
  registry_info.service_name = "my_service";
  registry_info.endpoint.ip = "127.0.0.1";
  registry_info.endpoint.port = 8080;
    
  trpc::TrpcNaming trpc_naming;
  int ret = trpc_naming.Register(registry_info);
  if (ret != 0) {
    // Handle registration failure situations
  }
}
```

### 4.2 Service discovery

```cpp
#include "trpc/naming/trpc_naming.h"

void DiscoverService() {
  trpc::TrpcSelectorInfo selector_info;
  selector_info.service_name = "my_service";

  trpc::TrpcEndpointInfo endpoint;
  trpc::TrpcNaming trpc_naming;
  int ret = trpc_naming.Select(selector_info, endpoint);
  if (ret != 0) {
    // Handle service discovery failure situations
  } else {
    // Use the found service endpoint for communication
  }
}
```

### 4.3 Traffic limiting operations

```cpp
#include "trpc/naming/trpc_naming.h"

void PerformLimiter() {
  trpc::LimitInfo limit_info;
  limit_info.service_name = "my_service";
  limit_info.caller_ip = "127.0.0.1";
  limit_info.caller_port = 12345;

  trpc::TrpcNaming trpc_naming;
  trpc::LimitRetCode ret_code = trpc_naming.ShouldLimit(&limit_info);
  if (ret_code != trpc::kLimitRetCodeAllow) {
    // Handle traffic limiting situations
  } else {
    // No traffic limiting triggered, continue with subsequent operations
  }
}
```
These examples demonstrate how to use the trpc_naming.h interface to complete service registration, service discovery, and traffic limiting operations. Please note that these examples assume that you have completed the development, registration, and configuration of custom naming service plugins. In actual use, you need to adjust these examples according to the actual needs and functionality of custom plugins.

## 5. Conclusion

This document introduces how to develop custom naming service plugins (including limiter plugins, service registration plugins, and service selector plugins) and demonstrates how to use the trpc_naming.h interface to complete service registration, service discovery, and traffic limiting operations.

The key steps to develop custom naming service plugins are:

1. Create a class, inherit from the corresponding naming service base class (such as trpc::Limiter, trpc::Registry, or trpc::Selector), and implement the methods in the base class.
2. Register the custom plugin with the corresponding naming service factory (such as trpc::LimiterFactory, trpc::RegistryFactory, or trpc::SelectorFactory).
3. Add custom plugin configurations to the configuration file.

After completing these steps, you can use the trpc_naming.h interface in the program to complete service registration, service discovery, and traffic limiting operations. We hope these examples and guidance will be helpful for you in developing custom naming service plugins in actual projects.
