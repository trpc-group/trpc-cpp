[TOC]
# 1 前言

tRPC-Cpp服务运行时必须提供正确的配置文件，配置文件包含运行框架所必备的配置项；同时通过配置文件的方式设置服务运行规则使得服务变更起来更加灵活，避免硬编码方式而导致发布繁琐等问题。

框架配置目前分为两大块:
1.框架内部基础配置如global，server，client部分，其中某些配置项取决于采用何种线程模型；
2.第三方插件，如log、prometheus、telemetry等；

目前框架配置项较多，为降低用户理解成本，便于快速上手，本片介绍精简配置版本，此版本介绍上述"1.框架内部基础配置"：包含能正确运行tRPC-Cpp服务的配置项最小集合。如需了解全部配置请查阅：[tRPC-Cpp 完整版配置](framework_config_full.md)。


# 2 说明
具体到精简配置项可以这样分类：
1.必须配置的global部分；
2.如果作为服务端接受请求，除了global部分，还必须配置server 部分；
3.如果使用客户端提供的工具(如Service Proxy)调用下游服务，除了global部分，还必须配置client 部分；

所以结合使用时，分为以下几种场景阐述:
1.服务端场景(主程序继承TrpcApp类并启动，服务仅接受请求就回包，不使用客户端提供的工具(如Service Proxy接口)调用下游服务。)，配置global 、server部分；
2.中转场景(在上述服务端基础之上，还需要使用客户端提供的工具(如Service Proxy)调用下游服务)，配置global、server、client ；
3.仅客户端：主程序没有继承TrpcApp类，需要自行初始化框架运行环境，如各种插件等；配置global和client部分(参考中转场景下删掉server部分配置即可)，更多的是代码层面的初始化，所以不讨论；

## 1. 服务端场景
根据线程模型的不同，又分为分离(或合并)线程和m:n协程(fiber)线程模型。
###  分离(或合并)线程

服务端精简配置如下:

```yaml
#全局配置(必须)
global:
  threadmodel:
    default:                            # 默认线程实现模式，即普通1:1线程
      - instance_name: default_instance # 线程实例唯一标识，可以配置多个default线程实例
        io_handle_type: separate        # separate分离模型(io/handle 运行在不同的线程上) 或者 merge合并模型(io/handle 运行在相同的线程上)  
        io_thread_num: 2                # 网络IO线程个数
        handle_thread_num: 6            # 业务逻辑线程个数

#服务端配置
server:
  app: test
  server: helloworld
  admin_port: 8888                      # 启动admin service
  admin_ip: 0.0.0.0
  service:
    - name: trpc.test.helloworld.Greeter
      protocol: trpc                    # 应用层协议比如: trpc/http/...
      network: tcp                      # 网络传输层协议比如: tcp/udp
      ip: 0.0.0.0                       # service绑定的ip
      port: 12345                       # service绑定的port     

#插件配置 
xxx
```
###  m:n协程(fiber)线程模型
只是线程模型的不同导致的配置差异：
```yaml
#全局配置(必须)
global:
  threadmodel:
    fiber:                              # 使用 Fiber(M:N协程) 线程模型
      - instance_name: fiber_instance   # 线程实例唯一标识，目前暂不支持配置多个Fiber线程实例
        concurrency_hint: 8             # 运行的Fiber Worker物理线程个数，建议配置为实际可用核数个数(否则读取系统配置)

#服务端配置
server:
  app: test
  server: helloworld
  admin_port: 8888                      # 启动admin service
  admin_ip: 0.0.0.0
  service:
    - name: trpc.test.helloworld.Greeter
      protocol: trpc                    # 应用层协议比如: trpc/http/...
      network: tcp                      # 网络传输层协议比如: tcp/udp
      ip: 0.0.0.0                       # service绑定的ip
      port: 12345                       # service绑定的port     

#插件配置 
xxx
```

## 2. 中转场景
在作为服务端基础之上，使用框架serviceproxy访问下游，所以同服务端场景一样，根据线程模型的不同，又分为分离(或合并)线程和m:n协程(fiber)线程模型的中转场景精简配置。

###  分离(或合并)线程

中转场景精简配置如下:
```yaml
#全局配置(必须)
global:
  threadmodel:
    default:
      - instance_name: default_instance
        io_handle_type: separate
        io_thread_num: 2
        handle_thread_num: 2

#服务端配置
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
         
#客户端配置 
client:
  service:
    - name: trpc.test.helloworld.Greeter
      target: 127.0.0.1:12345         # 直连模式时填写的ip:port列表，可以同时配置多个，如 127.0.0.1:10001,127.0.0.1:10002
      protocol: trpc                  # 应用层协议比如: trpc/http/...
      network: tcp                    # 网络传输层协议比如: tcp/udp
      selector_name: direct           # 路由选择插件，默认是direct直连模式即直接配置IP:port
        
#插件配置
xxx
```

###  m:n协程(fiber)线程模型
只是线程模型的不同导致的配置差异：
```yaml
#全局配置(必须)
global:
  threadmodel:
    fiber:                            # 使用 Fiber(M:N协程) 线程模型
      - instance_name: fiber_instance # 线程实例唯一标识，目前暂不支持配置多个Fiber线程实例        
        concurrency_hint: 8           # 运行的Fiber Worker物理线程个数，建议配置为实际可用核数个数(否则读取系统配置)

client:
  service:
    - name: trpc.test.helloworld.Greeter
      target: 127.0.0.1:12345         # 直连模式时填写的ip:port列表，可以同时配置多个，如 127.0.0.1:10001,127.0.0.1:10002
      protocol: trpc                  # 应用层协议比如: trpc/http/...
      network: tcp                    # 网络传输层协议比如: tcp/udp
      selector_name: direct           # 路由选择插件，默认是direct直连模式即直接配置IP:port
#插件配置
xxx
```