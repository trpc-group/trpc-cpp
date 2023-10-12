[中文](../zh/framework_config_lite.md)

# Overview

During the runtime of tRPC-Cpp services, it is necessary to provide a correct configuration file. This configuration file includes essential configuration options for the framework to operate. By using a configuration file, the service runtime rules can be set, making it more flexible to make changes to the service and avoiding issues such as cumbersome hardcoding during deployment.

The framework configuration is currently divided into two main parts:

1. Internal basic configurations of the framework, such as global, server, and client sections, where certain configuration options depend on the chosen threading model.
2. Third-party plugins, such as Prometheus, telemetry, etc.

Currently, there are many configuration options available for the framework. To reduce the learning curve and facilitate quick start, this document introduces a simplified configuration version, which covers the aforementioned "1. Internal basic configurations" - the minimal set of configuration options required to run tRPC-Cpp services correctly. For a complete understanding of all configurations, please refer to the[tRPC-Cpp framework_config_full](/docs/en/framework_config_full.md)。

# Instruction

To be specific, the simplified configuration items can be classified as follows:

1. Mandatory configuration in the global section.
2. If serving as a server to accept requests, in addition to the global section, the server section must also be configured.
3. If using client tools (such as Service Proxy) to call downstream services, in addition to the global section, the client section must also be configured.

Therefore, when combined, the following scenarios can be described:

1. Server scenario (the main program inherits the TrpcApp class and starts, the service only accepts requests and sends responses, without using client tools like Service Proxy to call downstream services). Configure the global and server sections.
2. Route scenario (building upon the server scenario, additionally using client tools like Service Proxy to call downstream services). Configure the global, server, and client sections.
3. Client-only scenario: The main program does not inherit the TrpcApp class and needs to initialize the framework runtime environment, including various plugins. Configure the global and client sections (refer to the relay scenario and remove the server section configuration). This scenario involves more code-level initialization, so it is not discussed further.

## Server scenario

According to different threadmodel, it can be further divided into the separate(or merge) threadmodel and the M:N coroutine (fiber) threadmodel.

* **Separate(or merge) threadmodel**

  The simplified server configuration is as follows:

  ```yaml
  global:
    threadmodel:
      default:                            # Default thread implementation mode, which is the 1:1 separate(or merge) threadmodel.
        - instance_name: default_instance # Unique identifier for thread instances, multiple default thread instances can be configured.
          io_handle_type: separate        # separate separation model (io/handle running on different threads) or merge merging model (io/handle running on the same thread)
          io_thread_num: 2                # Number of network IO threads
          handle_thread_num: 6            # Number of business logic threads
  
  server:
    app: test
    server: helloworld
    admin_port: 8888                      # Start admin service
    admin_ip: 0.0.0.0
    service:
      - name: trpc.test.helloworld.Greeter
        protocol: trpc                    # Application layer protocols such as: trpc/http/...
        network: tcp                      # Network transport layer protocols such as: tcp/udp
        ip: 0.0.0.0                       # IP address bound to the service
        port: 12345                       # Port bound to the service
  
  #plugin
  xxx
  ```

* **Fiber theadmodel**

  Configuration differences caused solely by different thread models:
  
  ```yaml
  global:
    threadmodel:
      fiber:                              # Fiber threadmodel
        - instance_name: fiber_instance   # Thread instance unique identifier, currently does not support configuring multiple Fiber thread instances
          concurrency_hint: 8             # Number of physical threads running Fiber Workers, it is recommended to configure it as the actual number of available cores (otherwise read system configuration)
  
  server:
    app: test
    server: helloworld
    admin_port: 8888                      # Start admin service
    admin_ip: 0.0.0.0
    service:
      - name: trpc.test.helloworld.Greeter
        protocol: trpc                    # Application layer protocols such as: trpc/http/...
        network: tcp                      # Network transport layer protocols such as: tcp/udp
        ip: 0.0.0.0                       # IP address bound to the service
        port: 12345                       # Port bound to the service
  
  #plugin
  xxx
  ```

## Route scenario

On top of serving as a server, using the framework serviceproxy to access downstream, just like the server-side scenario, based on different thread models, it is further divided into simplified configurations for both the separate(or merge) threadmodel and the m:n coroutine (fiber) thread model in the intermediate transfer scenario.

* **Separate(or merge) threadmodel**

  The simplified route scenario configuration is as follows:
  
  ```yaml
  global:
    threadmodel:
      default:
        - instance_name: default_instance
          io_handle_type: separate
          io_thread_num: 2
          handle_thread_num: 2
  
  server:
    app: test
    server: forword
    admin_port: 8889
    admin_ip: 0.0.0.0
    service:
      - name: trpc.test.forword.Forward
        protocol: trpc
        network: tcp
        ip: 0.0.0.0
        port: 12346
           
  client:
    service:
      - name: trpc.test.helloworld.Greeter
        target: 127.0.0.1:12345         # IP:port list when using direct connection mode, multiple can be configured simultaneously, such as 127.0.0.1:10001,127.0.0.1:10002
        protocol: trpc                  # Application layer protocol, such as trpc/http/...
        network: tcp                    # Network transport layer protocol, such as tcp/udp
        selector_name: direct           # Routing selection plugin, default is direct for direct connection mode, i.e., directly configuring IP:port
          
  #plugin
  xxx
  ```

* **Fiber theadmodel**

  Configuration differences caused solely by different thread models:
  
  ```yaml
  global:
    threadmodel:
      fiber:                              # Fiber threadmodel
        - instance_name: fiber_instance   # Thread instance unique identifier, currently does not support configuring multiple Fiber thread instances
          concurrency_hint: 8             # Number of physical threads running Fiber Workers, it is recommended to configure it as the actual number of available cores (otherwise read system configuration)
  
  client:
    service:
      - name: trpc.test.helloworld.Greeter
        target: 127.0.0.1:12345         # IP:port list when using direct connection mode, multiple can be configured simultaneously, such as 127.0.0.1:10001,127.0.0.1:10002
        protocol: trpc                  # Application layer protocol, such as trpc/http/...
        network: tcp                    # Network transport layer protocol, such as tcp/udp
        selector_name: direct           # Routing selection plugin, default is direct for direct connection mode, i.e., directly configuring IP:port
          
  #plugin
  xxx
  ```
