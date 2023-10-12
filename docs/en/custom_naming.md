[中文](../zh/custom_naming.md)

## Overview

The naming module of the tRPC-Cpp framework provides service discovery, service registration/heartbeat reporting, and rate limiting related functions, including selector, registry, and limiter plugins. Users can use the naming module's functions based on the tRPC client service proxy (combined with specific framework configuration items or service proxy configuration items) or through the external API interface of the naming module (located in trpc/naming/trpc_naming.h). Based on this, users can develop custom naming service plugins.

In this document, we will guide developers on how to develop custom naming service plugins.

## Develop custom naming service plugins

To implement a custom naming service plugin, you need to complete the following steps:

### Implement the naming service base class interface

First, the plugin developer needs to create a class that inherits from the corresponding naming service base class (such as trpc::Limiter, trpc::Registry, or trpc::Selector) and implement the methods in the base class. Next, we provide examples for custom rate limiter plugins (CustomLimiter), custom service registry plugins (CustomRegistry), and custom service selector plugins (CustomSelector).

### Custom rate limiter plugin example

- **custom_limiter.h**

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

- **custom_limiter.cc**

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

In this example, the CustomLimiter class inherits from the trpc::Limiter base class and implements the ShouldLimit() and FinishLimit() methods. Developers can modify this example according to actual needs to implement support for custom rate limiter plugins.

### Custom service registry plugin example

- **custom_registry.h**

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

- **custom_registry.cc**

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

In this example, the CustomRegistry class inherits from the trpc::Registry base class and implements the Register() and Unregister() methods. Developers can modify this example according to actual needs to implement support for custom service registry plugins.

### Custom service selector plugin example

- **custom_selector.h**

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

- **custom_selector.cc**

   ```cpp
   #include "custom_selector.h"
  
   namespace trpc::custom_selector {

     CustomSelector::CustomSelector() {
       // Initialization operations, such as passing configuration parameters, etc.
     }
     
     int CustomSelector::Select(const TrpcSelectorInfo& info, TrpcEndpointInfo& endpoint) {
       // ...
     }
   }
   ```

In this example, the CustomSelector class inherits from the trpc::Selector base class and implements the Select() method.

Developers can modify this example according to actual needs to implement support for custom service selector plugins.
Additionally, if users need to rely on the framework to automatically trigger rate limiting and route selection functions, they need to implement the corresponding Limiter and Selector plugins according to the [Custom Interceptor](./filter.md) document, so as to connect the rate limiting and route selection functions to the entire framework process.

### Register custom naming service plugins

After creating the custom naming service plugins, you need to register them with the corresponding naming service factory.

1. For the server-side scenario, users need to register in the TrpcApp::RegisterPlugins function during service startup:

   ```cpp
   class HelloworldServer : public ::trpc::TrpcApp {
    public:
     // ...
     int RegisterPlugins() override {
       // Register Limiter plugin and its corresponding server/client filter
       TrpcPlugin::GetInstance()->RegisterLimiter(MakeRefCounted<CustomLimiter>());
       TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<CustomLimiterClientFilter>());
       TrpcPlugin::GetInstance()->RegisterServerFilter(std::make_shared<CustomLimiterServerFilter>());
   
       // Register Registry plugin
       TrpcPlugin::GetInstance()->RegisterRegistry(MakeRefCounted<CustomSelector>());
   
       // Register Selector plugin and its corresponding filter
       TrpcPlugin::GetInstance()->RegisterSelector(MakeRefCounted<CustomSelector>());
       TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<CustomSelectorFilter>());
   
       return 0;
     }
   };
   ```

2. For the pure client-side scenario, you need to register after the framework configuration initialization and before the start of other framework modules:

   ```cpp
   int main(int argc, char* argv[]) {
     ParseClientConfig(argc, argv);
   
     // Register Limiter plugin and its corresponding client filter
     TrpcPlugin::GetInstance()->RegisterLimiter(MakeRefCounted<CustomLimiter>());
     TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<CustomLimiterClientFilter>());
   
     // Register Selector plugin and its corresponding filter
     TrpcPlugin::GetInstance()->RegisterSelector(MakeRefCounted<CustomSelector>());
     TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<CustomSelectorFilter>());
   
     return ::trpc::RunInTrpcRuntime([]() { return Run(); });
   }
   ```

**Note that the plugins and interceptors only need to be constructed, and the framework will automatically call their `Init` functions during startup.**

## Use the trpc_naming.h interface

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

In this example, we added the names of the custom plugins (custom_registry, custom_selector, custom_limiter) to the corresponding configuration items. The framework will parse the configuration file and use the corresponding plugins accordingly.

It should be noted that the specific configuration parsing process is completed by the framework, and you only need to ensure that the plugin names in the configuration file match the names of your custom plugins.

## Use custom naming service plugins

After completing the development, registration, and configuration of custom naming service plugins, you can use these plugins in your program to complete service registration, service discovery, and rate limiting operations. Here are examples of using the interfaces provided by trpc_naming.h to perform operations:

### Service registration

```cpp
#include "trpc/naming/trpc_naming.h"

void RegisterService() {
  trpc::TrpcRegistryInfo registry_info;
  registry_info.service_name = "my_service";
  registry_info.endpoint.ip = "127.0.0.1";
  registry_info.endpoint.port = 8080;
    
  int ret = trpc::naming::Register(registry_info);
  if (ret != 0) {
    // Handle registration failure
  }
}
```

### Service discovery

```cpp
#include "trpc/naming/trpc_naming.h"

void DiscoverService() {
  trpc::TrpcSelectorInfo selector_info;
  selector_info.service_name = "my_service";

  trpc::TrpcEndpointInfo endpoint;
  int ret = trpc::naming::Select(selector_info, endpoint);
  if (ret != 0) {
    // Handle service discovery failure
  } else {
    // Use the found service endpoint for communication
  }
}
```

### Rate limiting operation

```cpp
#include "trpc/naming/trpc_naming.h"

void PerformLimiter() {
  trpc::LimitInfo limit_info;
  limit_info.service_name = "my_service";
  limit_info.caller_ip = "127.0.0.1";
  limit_info.caller_port = 12345;

  trpc::LimitRetCode ret_code = trpc::naming::ShouldLimit(&limit_info);
  if (ret_code != trpc::kLimitRetCodeAllow) {
    // Handle rate limiting
  } else {
    // Continue with subsequent operations without triggering rate limiting
  }
}
```

These examples demonstrate how to use the interfaces provided by trpc_naming.h to complete service registration, service discovery, and rate limiting operations. Please note that these examples assume that you have completed the development, registration, and configuration of custom naming service plugins. In actual use, you need to adjust these examples according to your actual needs and the functionality of your custom plugins.

## Conclusion

This document introduces how to develop custom naming service plugins (including rate limiter plugins, service registry plugins, and service selector plugins) and demonstrates how to use the interfaces provided by trpc_naming.h to complete service registration, service discovery, and rate limiting operations.

The key steps to develop custom naming service plugins are as follows:

1. Create a class that inherits from the corresponding naming service base class (such as trpc::Limiter, trpc::Registry, or trpc::Selector) and implement the methods in the base class. And implement the corresponding filter.
2. Register the custom plugin and filter with the framework.
3. Add the custom plugin configuration in the configuration file.

After completing these steps, you can use the interfaces provided by trpc_naming.h in your program to complete service registration, service discovery, and rate limiting operations.
