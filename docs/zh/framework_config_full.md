
[English](../en/framework_config_full.md)

# 前言

与精简版本配置相比，完整版配置包含了一些自定义的高级配置。

# 说明

tRPC-Cpp 框架的全部配置文件定义：

```yaml
#全局配置
global:
  namespace: xxx                                                  #环境类型，naming插件使用
  env_name: xxx                                                   #配置名称，naming插件使用
  container_name: xxx                                             #运行容器名称
  local_ip: xxx                                                   #本地IP，用于从框架配置拿到本机ip
  local_nic: xxx                                                  #本地网卡名，用于通过网卡名获取ip(优先用local_ip，没有则用网卡获取local_ip)
  periphery_task_scheduler_thread_num: 1                          #业务使用的PeripheryTaskScheduler定时器配置多少线程处理（默认1个）
  inner_periphery_task_scheduler_thread_num: 2                    #框架内部使用的PeripheryTaskScheduler定时器配置多少线程处理（默认2个，一般无需调整）
  enable_runtime_report: true                                     #开启框架Runtime信息上报，比如CPU使用率，TCP链接个数等等。
  report_runtime_info_interval: 60000                             #框架Runtime信息上报间隔，单位为milliseconds
  heartbeat:
    enable_heartbeat: true                                        #开启心跳上报，默认true，开启后，能定期上报心跳到名字服务、检测线程僵死、上报线程的queue size特性指标
    thread_heartbeat_time_out: 60000                              #检测工作线程僵死的超时时间（ms）
    heartbeat_report_interval: 3000                               #工作线程僵死检测间隔时间（ms）
  buffer_pool:                                                    #内存池配置
    mem_pool_threshold: 536870912                                 #内存池阈值大小，默认512M
    block_size: 4096                                              #内存池块大小，默认4k
  enable_set: Y                                                   #是否启用set
  full_set_name: app.sh.1                                         #set名，常用格式为"应用名.地区.分组id"三段式
  thread_disable_process_name: true                               #默认为true，即框架线程名称设置为框架内部指定名称（比如，在Fiber下，为FiberWorker）。如果期望线程名称和进程名称一致，请设置为false（当前在Fiber模式生效）
  threadmodel:
    #这里可以配置多种线程模型，分离(或合并)线程模型和Fiber线程模型均可以，这里仅做展示，可需要调整
    default:                                                      #默认线程实现模式，即普通1:1线程。
      - instance_name: default_instance                           #线程模型实例名称，允许有多个线程模型实例
        io_handle_type: separate                                  #运行模式，separate/merge二选一
        io_thread_num: 2                                          #io线程个数，无论separate还是merge都需要此配置项；如不填，为随机大于0的数，所以建议填写。如填0，会兼容为1；
        io_thread_task_queue_size: 65536                          #io线程任务队列的大小，数值必须是2的幂；如果不填或者填0，会兼容为65536；如果填的数值不是2的幂，框架会做兼容，此时队列大小比用户设置的要大。
        handle_thread_num: 6                                      #handle线程个数，对于merge模式不生效；separate模式，必填。如不填，为随机大于0的数，所以建议填写。如填0，会兼容为1；
        handle_thread_task_queue_size: 65536                      #handle线程任务队列的大小，对于merge模式不生效。数值必须是2的幂；如果不填或者填0，会兼容为65536；如果填的数值不是2的幂，框架会做兼容，此时队列大小比用户设置的要大。
        scheduling:
          scheduling_name: non_fiber                              #业务逻辑线程调度器名称
          local_queue_size: 10240                                 #每个handle线程的私有任务队列大小
          max_timer_size: 20480                                   #每个handle线程最大定时器个数
        io_cpu_affinitys: "0-1"                                   #将io线程绑定到核0和1上
        handle_cpu_affinitys: "2-8"                               #将 handle 线程绑定到核2-8这6个核上，仅在分离模式生效
        disallow_cpu_migration: false                             #是否严格绑核，true：表示每个线程只绑定到一个核上，此时配置的核酸必须大于或者等于相应的线程数；false：每一个线程可以绑定到多个核上

        enable_async_io: false                                    #是否使用async_io
        io_uring_entries: 1024                                    #io_uring queue大小
        io_uring_flags: 0                                         #io_uring标识
    #fiber线程模型
    fiber:
      - instance_name: fiber_instance
        concurrency_hint: 8                                       #建议配置，表示共创建多少个fiber worker物理线程来运行fiber任务。如果不配置默认值是读取/proc/cpuinfo个数(容器场景下实际可用核数可能远低于/proc/cpuinfo值，会造成fiber worker线程频繁切换影响性能)，所以建议此值和实际可用核数相同。
        scheduling_group_size: 4                                  #建议配置，表示每个调度组(为了减小竞争，框架引入多调度组来管理fiber worker物理线程)共有多少个fiber worker物理线程。如果不配置默认框架会依据concurrency_hint值和策略自动创建一个或者多个调度组。如果希望当前只有一个调度组，将此值配置同concurrency_hint一样即可。如果希望有多个调度组，可以参考展示配置项:表示每个调度组有4个fiber worker物理线程，共有2个调度组。
        reactor_num_per_scheduling_group: 1                       #表示每个调度组共有多少个reactor模型，如果不配置默认值为1个。针对io比较重的场景，可以适当调大此参数，但也不要过高。 
        reactor_task_queue_size: 65536                            #表示reactor任务队列的大小
        fiber_stack_size: 131072                                  #表示fiber栈大小，如果不配置默认值为128K。如果需要申请的栈资源较大，可以调整此值
        fiber_run_queue_size: 131072                              #表示每个调度组的Fiber运行队列的长度，必须是2幂次，建议和可用Fiber分配的个数相同或稍大。
        fiber_pool_num_by_mmap: 30720                             #表示通过mmap分配fiber stack的个数
        numa_aware: false                                         #表示是否启用numa，如果不配置默认值为false。配置为true表示框架会将调度组绑定到cpu nodes(前提是硬件支持numa)，false表示由操作系统调度线程运行在线程运行在哪个cpu上。
        fiber_worker_accessible_cpus: 0-4,6,7                     #表示需要指定运行在特定的cpu IDs，如果不配置默认值为空。如果希望指定，参考当前展示配置项：表示指定从0到4，还有6,7这几个cpu ID
        fiber_worker_disallow_cpu_migration: false                #表示是否允许fiber_worker在不同cpu上运行，也就是是否绑核，如果不配置默认值为false也就是默认不绑核。
        work_stealing_ratio: 16                                   #表示不同调度组之间任务窃取的比例，如果不配置默认值是16，表示按照16%比例进行任务窃取。
        cross_numa_work_stealing_ratio: 0                         #表示numa架构不同node之间偷取任务频率(v1调度器版本实现支持)，如果不配置默认值为0表示不开启(开启会比较影响效率，建议实际测试后再开启)
        fiber_stack_enable_guard_page: true                       #是否启用fiber栈保护，如果不配置默认值为true，建议启用。
        fiber_scheduling_name: v1                                 #表示fiber运行/切换的调度器实现，目前提供两种调度器机制的实现：v1/v2，如果不配置默认值是v1版本即原来fiber调度的实现，v2版本是参考taskflow的调度实现
  
  tvar:
    #tvar的相关配置，详情请参考《tvar》文档

  rpcz:
    #rpcz的相关配置，详情请参考《rpcz》文档


#服务端配置
server:                                                             
  app: test                                                       #业务名
  server: helloworld                                              #业务的模块名
  bin_path: /usr/local/trpc/bin/                                  #程序执行路径，用于从框架配置拿到程序运行路径
  conf_path: /usr/local/trpc/conf/                                #配置文件所在路径，用于从框架配置拿到配置文件路径
  data_path: /usr/local/trpc/data/                                #数据文件所在路径，用于从框架配置拿到数据文件路径
  enable_server_stats: false                                      #是否开启指标(如连接数，请求数和延时)的统计和输出(默认不)，定期输出到框架日志中
  server_stats_interval: 60000                                    #即指标统计的输出周期(单位ms，不配置默认60s)
                                                                  #admin是程序内置的http服务，提供管理接口、运行状态查询等功能。默认不开启，必须配置了下边两个配置项才会开启
  admin_port: 7897                                                #admin监听端口
  admin_ip: ${trpc_admin_ip}                                      #admin监听ip
  admin_idle_time: 60000                                          #admin空闲连接清理时间，框架默认60s，如果业务注册了处理时间超过60s的逻辑，适当调大此值以得到响应
  stop_max_wait_time: 3000                                        #设置max wait timeout(ms) 避免无法正常退出
  service:                                                        #业务服务提供的service，可以有多个
    - name: trpc.test.helloworld.Greeter                          #service名称，需要按照这里的格式填写，第一个字段默认为trpc，第二、三个字段为上边的app和server配置，第四个字段为用户定义的service_name
      protocol: trpc                                              #应用层协议：trpc http等
      socket_type: net                                            #socket类型：默认net(网络套接字)，也支持unix(unix domain socket)
      network: tcp                                                #网络监听类型  tcp udp，socket_type=unix时无效
      ip: xxx                                                     #监听ip，socket_type=unix时无效
      nic: xxx                                                    #监听网卡名，用于通过网卡名获取ip(优先用ip，没有则用网卡获取ip)
      port: 10001                                                 #监听port，socket_type=unix时无效
      is_ipv6: false                                              #ip是否是ipv6格式
      unix_path: trpc_unix.socket                                 #socket_type=unix时生效，指定unix socket的绑定地址
      max_conn_num: 100000                                        #最大连接数目
      queue_size: 2000000                                         #接收队列大小
      queue_timeout: 5000                                         #请求在接收队列的超时时间，ms
      timeout: 1000                                               #session超时时间，ms
      idle_time: 60000                                            #连接空闲超时时间，ms
      max_packet_size: 10000000                                   #请求包大小限制
      disable_request_timeout: false                              #是否启用全链路超时，默认启用 
      share_transport: true                                       #当时多个service的"ip/port/protocol"相同时，是否共享transport，默认启用
      recv_buffer_size: 10000000                                  #每次从网络socket读取数据最大长度，如果设置为0标识不设置限制
      send_queue_capacity: 0                                      #Fiber场景下使用，表示发送网络数据时，io发送队列能cached的最大长度，如果设置为0标识不设置限制
      send_queue_timeout: 3000                                    #Fiber场景下使用，表示发送网络数据时io发送队列的超时时间 
      threadmodel_instance_name: default_instance                 #使用的线程模型实例名，为global->threadmodel->instance_name内容
      accept_thread_num: 1                                        #绑定端口的线程个数，如果大于1，需要指定编译选项.
      stream_max_window_size: 65535                               #默认窗口值为65535，0代表关闭流控，除此之外，如果设置小于65535将不会生效
      stream_read_timeout: 32000                                  #从流上读取消息超时，单位：毫秒，默认为32000ms
      filter:                                                     #service级别的filter列表，只针对当前service生效
        - xxx                                                     #具体的filter名称
  filter:                                                         #服务端调用执行过程中的拦截器列表(针对server下的所有service生效)
    - xxx

#客户端配置
client:                                                          
  service:                                                        #service_proxy配置，可有多个
    - name: trpc.test.helloworld.Router                           #service_proxy配置唯一标识，用户代码中给service proxy设定name后，框架会根据name到配置文件中寻找对应配置，因此这里需要设置为对应的service proxy name
      selector_name: direct                                       #路由选择使用的名字服务，使用直连则为：direct,域名访问为domain
      target: 127.0.0.1:10000,127.0.0.1:10000                     #被调service名称，有多种情况：直连时，为一组逗号隔开的ip:port如：127.0.0.1:10000,127.0.0.1:10000；域名方式填域名即可(可以带端口)
      namespace: xxx                                              #环境类型，naming插件使用
      protocol: trpc                                              #协议
      timeout: 1000                                               #调用超时时间，ms
      network: tcp                                                #网络类型
      conn_type: long                                             #连接类型，长连接/短连接
      threadmodel_instance_name: default_instance                 #使用的线程模型实例名，含义同server->service->threadmodel_instance_name
      callee_name: xxx                                            #被调服务名称，如果是空的话，名称设置为同name配置项值
      callee_set_name: app.sh.1                                   #被调服务的set名，用于指定set调用
      is_conn_complex: true                                       #是否使用连接复用；如果设置为false，表示使用连接池；如果设置为true，该协议本身如果支持连接复用(如trpc)，会使用连接复用，如果该协议本身不支持连接复用(如http)，仍会使用连接池
      max_conn_num: 1                                             #连接池模式下最大连接个数，对连接复用模式无效 
      idle_time: 50000                                            #连接空闲超时时间(ms)
      max_packet_size: 10000000                                   #请求包大小限制
      load_balance_name: xxx                                      #需要使用的负载均衡类型
      is_reconnection: true                                       #只适用于于连接复用的场景，决定是否定时剔除空闲连接后需要新建连接.
      allow_reconnect: true                                       #在固定链接场景，是否可以支持重新建立连接      
      recv_buffer_size: 10000000                                  #每次ServiceProxy从网络socket读取数据最大长度，如果设置为0标识不设置限制
      send_queue_capacity: 0                                      #Fiber场景下使用，表示发送网络数据时，io发送队列能cached的最大长度，如果设置为0标识不设置限制
      send_queue_timeout: 3000                                    #Fiber场景下使用，表示发送网络数据时io发送队列的超时时间 
      stream_max_window_size: 65535                               #默认窗口值为65535，0代表关闭流控，除此之外，如果设置小于65535将不会生效
      request_timeout_check_interval: 10                          #IO/Handle分离及合并模式下的请求超时检测间隔，默认为10ms。如果设置的超时时间比较小（如小于10ms）的话，可对应调小这个值
      disable_servicerouter: false                                #是否禁用服务规则路由，默认不禁用
      support_pipeline: false                                     #是否启用pipeline，默认关闭，当前仅针对redis协议有效。调用redis-server时建议开启，可以获得更好的性能。
      fiber_pipeline_connector_queue_size:                        #FiberPipelineConnector队列大小，如果内存占用加大可以减小此配置
      connect_timeout: 0                                          #是否开启connect连接超时检测，默认不开启(为0表示不启用)。当前仅支持IO/Handle分离及合并模式
      filter:                                                     #service级别的filter列表，只针对当前service生效
        - xxx                                                     #具体的filter名称
      redis:                                                      #调用redis的相关配置，详情请参考《访问redis协议服务》文档
      ssl:                                                        #ssl相关配置，用于https，详情请参考《访问http(s)协议服务》文档
  filter:                                                         #客户端调用执行过程中的拦截器列表
    - xxx                                                         #客户端调用执行过程中的拦截器列表(针对client下的所有service生效)

#插件配置，这部分可详见各个插件的文档说明
plugins:
  log:     #日志插件，参考具体插件文档
    xxx
  
  metrics: #metrics插件配置，参考具体插件文档
    xxx
  
  tracing: #tracing插件配置，参考具体插件文档
    xxx
  
  naming:  #名字服务插件，参考具体插件文档
    xxx
  
  telemetry:  #telemetry插件，参考具体插件文档
    xxx
  
  config: #配置中心插件，参考具体插件文档
    xxx
```
