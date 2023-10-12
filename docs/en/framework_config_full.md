
[中文](../zh/framework_config_full.md)

# Overview

Compared to the simplified version configuration, the complete configuration includes some custom advanced configurations

# Instruction

```yaml
#全局配置
global:
  namespace: xxx                                                  #Environment type, naming plugin usage
  env_name: xxx                                                   #Environment name, naming plugin usage
  container_name: xxx 
  local_ip: xxx                                                   #local_ip，to obtain the local IP address from the framework configuration.
  local_nic: xxx                                                  #Local network interface name, used to obtain the IP address by the interface name (preferably using local_ip, if not available, use the interface to obtain local_ip).
  periphery_task_scheduler_thread_num: 1                          #PeripheryTaskScheduler thread count（default 1）
  inner_periphery_task_scheduler_thread_num: 2                    #Inner PeripheryTaskSchedulerthread count（default 2）
  enable_runtime_report: true                                     #Enable framework runtime information reporting, such as CPU usage, number of TCP connections, and so on
  report_runtime_info_interval: 60000                             #The interval for framework runtime information reporting is specified in milliseconds
  heartbeat:
    enable_heartbeat: true                                        #Enable heartbeat reporting, default is true. When enabled, it periodically reports heartbeats to the naming service, detects thread deadlocks, and reports the queue size of the reporting thread as a performance metric.
    thread_heartbeat_time_out: 60000                              #heartbeat timeout（ms）
    heartbeat_report_interval: 3000                               #heartbeat check interval（ms）
  buffer_pool:                                                    #buffer_pool
    mem_pool_threshold: 536870912                                 #mem_pool_threshold，default as 512M
    block_size: 4096                                              #block_size，default as 4k
  enable_set: Y                                                   #set
  full_set_name: app.sh.1                                         #set name
  thread_disable_process_name: true                               #If you want to set the thread name to a specific name specified within the framework (e.g., "FiberWorker" in Fiber mode), set it to true. If you want the thread name to be the same as the process name, set it to false (currently effective in Fiber mode)
  threadmodel:
    #Here, you can configure multiple thread models, including the separate(or merge) thread model and the Fiber thread model. This is just for demonstration purposes and may need to be adjusted based on your specific needs
    default:                                                      #The default thread implementation mode is the regular 1:1 thread model
      - instance_name: default_instance                           #The thread model instance name allows for multiple instances of thread models
        io_handle_type: separate                                  #The execution mode can be either "separate" or "merge"
        io_thread_num: 2                                          #io_thread_num
        io_thread_task_queue_size: 65536                          #io_thread_task_queue_size
        handle_thread_num: 6                                      #handle_thread_num
        handle_thread_task_queue_size: 65536                      #handle_thread_task_queue_size
        scheduling:
          scheduling_name: non_fiber                              #scheduling_name
          local_queue_size: 10240                                 #local_queue_size
          max_timer_size: 20480                                   #max_timer_size
        max_reactor_timer_size: 10240                             #max_reactor_timer_size
        io_cpu_affinitys: "0-1"                                   #Bind the I/O threads to cores 0 and 1.
        handle_cpu_affinitys: "2-8"                               #Bind the I/O threads to cores 2 ~ 8.
        disallow_cpu_migration: false                             #Whether to bind cores strictly, true: indicates that each thread is bound to a single core, in which case the configured number of cores must be greater than or equal to the corresponding number of threads; false: each thread can be bound to multiple cores.
        
        enable_async_io: false                                    #async_io
        io_uring_entries: 1024                                    #io_uring queue size
        io_uring_flags: 0                                         #io_uring flag

    fiber:
      - instance_name: fiber_instance
        concurrency_hint: 8                                       #Suggested configuration, indicating the total number of physical fiber worker threads to run fiber tasks. If not configured, the default value is to read the number of /proc/cpuinfo (in container scenarios, the actual number of available cores may be much lower than the value in /proc/cpuinfo, which can cause frequent thread switching and impact performance). Therefore, it is recommended to set this value to be the same as the actual number of available cores.
        scheduling_group_size: 4                                  #Suggested configuration, indicating the number of fiber worker threads per scheduling group (to reduce contention, the framework introduces multiple scheduling groups to manage physical fiber worker threads). If not configured, the framework will automatically create one or more scheduling groups based on the concurrency_hint value and strategy. If you want to have only one scheduling group, you can set this value the same as concurrency_hint. If you want to have multiple scheduling groups, you can refer to the example configuration: indicating each scheduling group has 4 fiber worker threads, with a total of 2 scheduling groups.
        reactor_num_per_scheduling_group: 1                       #It indicates the number of reactor models per scheduling group. If not configured, the default value is 1. For scenarios with heavy I/O, you can increase this parameter appropriately, but avoid setting it too high.
        reactor_task_queue_size: 65536                            #reactor_task_queue_size
        fiber_stack_size: 131072                                  #fiber_stack_size default 128K
        fiber_run_queue_size: 131072                              #fiber_run_queue_size
        fiber_pool_num_by_mmap: 30720                             #fiber_pool_num_by_mmap
        numa_aware: false                                         #numa_aware
        fiber_worker_accessible_cpus: 0-4,6,7                     #It indicates the need to specify running on specific CPU IDs. If not configured, the default value is empty. If you want to specify, refer to the current example configuration: specifying CPU IDs from 0 to 4, as well as 6 and 7.
        fiber_worker_disallow_cpu_migration: false                #It indicates whether to allow fiber workers to run on different CPUs, i.e., whether to bind cores. If not configured, the default value is false, which means cores are not bound by default.
        work_stealing_ratio: 16                                   #It represents the proportion of task stealing between different scheduling groups. If not configured, the default value is 16, indicating task stealing is performed in a 16% proportion.
        cross_numa_work_stealing_ratio: 0                         #It represents the frequency of task stealing between different nodes in a NUMA architecture.
        fiber_stack_enable_guard_page: true                       #fiber_stack_enable_guard_page
        fiber_scheduling_name: v1                                 #fiber_scheduling_name
  
  tvar:# see [tvar]

  rpcz:# see [rpcz]


server:                                                             
  app: test                                                       
  server: helloworld                                              
  bin_path: /usr/local/trpc/bin/                                 
  conf_path: /usr/local/trpc/conf/                               
  data_path: /usr/local/trpc/data/                               
  enable_server_stats: false                                      
  server_stats_interval: 60000                                 
                                                                  #Admin is a built-in HTTP service in the program that provides management interfaces, runtime status queries, and other functionalities. It is not enabled by default and requires the configuration of the following two options to be enabled.
  admin_port: 7897                                                
  admin_ip: 0.0.0.0                                   
  admin_idle_time: 60000                                          #admin_idle_time
  stop_max_wait_time: 3000                                        #set max wait timeout(ms) when stop incase cannot stop
  service:
    - name: trpc.test.helloworld.Greeter                          
      protocol: trpc                                               
      socket_type: net                                       
      network: tcp                                             
      ip: xxx                                                 
      nic: xxx                                           
      port: 10001                                          
      is_ipv6: false                                             
      unix_path: trpc_unix.socket                           
      max_conn_num: 100000                              
      queue_size: 2000000                                  
      queue_timeout: 5000                              
      timeout: 1000
      idle_time: 60000 
      max_packet_size: 10000000
      disable_request_timeout: false
      share_transport: true                                       #When multiple services have the same "ip/port/protocol," whether to share the transport, enabled by default.
      recv_buffer_size: 10000000                                  #The maximum length of data to read from the network socket each time. Setting it to 0 indicates no limit is set.
      send_queue_capacity: 0                                      #Used in Fiber scenarios, it represents the maximum length of the IO send queue that can be cached when sending network data. Setting it to 0 indicates no limit is set.
      send_queue_timeout: 3000                                    #Used in Fiber scenarios, It represents the timeout duration for the IO send queue when sending network data.
      threadmodel_instance_name: default_instance 
      accept_thread_num: 1 
      stream_max_window_size: 65535                               #The default window value is 65535. 0 represents disabling flow control. Additionally, if set to a value less than 65535, it will not take effect.
      stream_read_timeout: 32000                                  #stream_read_timeout
      filter:                                                     #The filter list at the service level, only effective for the current service.
        - xxx          
  filter:                                                         #The list of interceptors during the execution process of server-side invocations (effective for all services under the server).
    - xxx

client:                                                          
  service:                                                       
    - name: trpc.test.helloworld.Router                           
      selector_name: direct                                       #selector_name，such as direct、domain
      target: 127.0.0.1:10000,127.0.0.1:10000                     #The name of the called service can have multiple formats：For direct connections, it is a group of IP:port pairs separated by commas, such as 127.0.0.1:10000,127.0.0.1:10000;For domain-based connections, simply provide the domain name (optionally with a port).
      namespace: xxx                                         
      protocol: trpc 
      timeout: 1000  
      network: tcp 
      conn_type: long 
      threadmodel_instance_name: default_instance 
      callee_name: xxx                                            #callee_name，If it is empty, the name is set to the same value as the 'name' configuration item.
      callee_set_name: app.sh.1                                   #callee_set_name，when call using set
      is_conn_complex: true                                       #If set true，and protocol support (such as protocol trpc)，will use conn_complex，otherwise use conn_pool      
      max_conn_num: 1                                             #max_conn_num
      idle_time: 50000 
      max_packet_size: 10000000 
      load_balance_name: xxx 
      is_reconnection: true                                       #Whether to reconnect after the idle connection is disconnected when reach connection idle timeout.
      allow_reconnect: true                                       #Whether to support reconnection in fixed connection mode, the default value is true. 
      recv_buffer_size: 10000000                                  #When the `ServiceProxy` reads data from the network socket,the maximum data length allowed to be received at one time,If set 0, not limited
      send_queue_capacity: 0                                      #When sending network data, the maximum data length of the io-send queue has cached ,use in fiber runtime, if set 0, not limited
      send_queue_timeout: 3000                                    #When sending network data, the timeout(ms) of data in the io-send queue,use in fiber runtime
      stream_max_window_size: 65535                               #Under streaming, sliding window size(byte) for flow control in recv-side
      request_timeout_check_interval: 10                          #The interval(ms) of check request whether has timeout
      disable_servicerouter: false                                #Whether to disable service rule-route
      support_pipeline: false                                     #Whether support connection pipeline.Connection pipeline means that you can multi-send and multi-recv in ordered on one connection
      fiber_pipeline_connector_queue_size:                        #The queue size of FiberPipelineConnector
      connect_timeout: 0                                          #The timeout(ms) of check connection establishment
      filter:                                                     #only effective for the current service.
        - xxx
      redis:                                                      #see [call redis protocol]
      ssl:                                                        #see [call https protocol]
  filter:                                                         #The list of interceptors during the execution process of client-side invocations (effective for all services under the client).
    - xxx

#see doc of this plugin
plugins:
  log:
    xxx
  
  metrics:
    xxx
  
  tracing:
    xxx
  
  naming:
    xxx
  
  telemetry:
    xxx
  
  config:
    xxx
```
