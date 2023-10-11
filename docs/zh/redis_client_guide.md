
# 1 背景

Redis是当今最流行的内存数据库之一，广泛应用于各项业务中。tRPC-Cpp目前仅提供了对Redis客户端协议的支持，所以本文中提到的"Redis调用"、"Redis使用"、"Redis开发"均指客户端方式访问下游Redis服务。在功能层面tRPC-Cpp提供了原生Redis协议的几乎全部能力(sub/pub/选库等少量命令暂不支持）。

本文主要介绍如何使用tRPC-Cpp框架调用一个Redis服务。本文只对Redis协议特有的一些属性详细说明，共性的部分一笔带过。阅读本文之前，请认真阅读[tRPC-Cpp客户端开发向导](./client_guide.md)。

## 1 Redis协议
Redis客户端使用RESP（Redis的序列化协议）协议与Redis服务端进行通信。虽然该协议是专门为Redis设计的，但是该协议也可以用于其他客户端-服务器（Client-Server）软件项目。Redis协议是一种文本协议，客户端和服务器发送的命令或数据以 "\r\n（CRLF）"结尾。有些开发者会误以为Redis协议是以空格分隔的，在使用tPRC-Cpp客户端的时候喜欢自己拼Redis访问命令。在不太了解Redis协议本身的时候推荐使用 trpc::redis::cmdgen{}.xxxx系列接口生成对应的[command](https://redis.io/commands)，直接把生成的command传入proxy对应接口访问Redis服务端。

## 2 接口形式
目前通过trpc::redis::RedisServiceProxy类发起Redis访问，对外提供的接口分两种：同步调用和异步调用。
### 同步接口
| 类          |  接口名称 |  功能 |参数 |返回值|备注|
| :-------------------| :-------------------------------------------------------------------------------:|:---------------------|:------------------------|:-----------|:---------------: |
|RedisServiceProxy | Command(const ClientContextPtr& context, Reply* reply, const std::string& cmd) |  执行Redis命令返回结果|context 客户端上下文 <br> reply  Redis执行结果 <br> cmd Redis命令 | Status | |
|RedisServiceProxy | Command(const ClientContextPtr& context, Reply* reply, std::string&& cmd) | 执行Redis命令返回结果|context 客户端上下文 <br> reply  Redis执行结果 <br> cmd Redis命令  | Status |   |
|RedisServiceProxy | Command(const ClientContextPtr& context, Reply* reply, const char* format, ...) | 执行Redis命令返回结果| context 客户端上下文 <br> reply  Redis执行结果 <br> format 通过format的方式生成Redis命令 | Status |容易出错 |
|RedisServiceProxy | CommandArgv(const ClientContextPtr& context, const Request& req, Reply* reply) | 执行Redis命令返回结果| context 客户端上下文 <br> req 请求 <br> reply  Redis执行结果   | Status |   |
|RedisServiceProxy | CommandArgv(const ClientContextPtr& context, Request&& req, Reply* reply) | 执行Redis命令返回结果| context 客户端上下文 <br> req 请求 <br> reply  Redis执行结果   |  Status |  |
|RedisServiceProxy | Command(const ClientContextPtr& context, const std::string& cmd) | 执行Redis命令不返回结果| context 客户端上下文 <br> cmd Redis命令 |  Status | 单向调用接口，谨慎使用 |
|RedisServiceProxy | Command(const ClientContextPtr& context, std::string&& cmd) | 执行Redis命令不返回结果| context 客户端上下文 <br> cmd Redis命令 |  Status |单向调用接口，谨慎使用  |

### 异步接口
| 类          |  接口名称 |  功能 |参数 |返回值|备注|
| :-------------------| :-------------------------------------------------------------------------------:|:---------------------|:------------------------|:-----------|:---------------: |
|RedisServiceProxy | AsyncCommand(const ClientContextPtr& context, const std::string& cmd) |  执行Redis命令返回结果|context 客户端上下文 <br>  cmd Redis命令 | Future<Reply> | |
|RedisServiceProxy | AsyncCommand(const ClientContextPtr& context, std::string&& cmd) | 执行Redis命令返回结果|context 客户端上下文 <br>  cmd Redis命令  | Future<Reply> |  |
|RedisServiceProxy | AsyncCommand(const ClientContextPtr& context, const char* format, ...) | 执行Redis命令返回结果| context 客户端上下文 <br> format 通过format的方式生成Redis命令 | Future<Reply> |容易出错 |
|RedisServiceProxy | AsyncCommandArgv(const ClientContextPtr& context, const Request& req) | 执行Redis命令返回结果| context 客户端上下文 <br> req 请求    | Future<Reply> |   |
|RedisServiceProxy | AsyncCommandArgv(const ClientContextPtr& context, Request&& req) | 执行Redis命令返回结果| context 客户端上下文 <br> req 请求 |  Future<Reply> |  |

无论同步接口还是异步接口，出于性能考虑推荐使用同名的右值接口，cmd或者req调用之后还需要使用的场景才使用左值接口。

## 3 返回值
同步接口结束的返回值类型为Status，通过reply出参获取Redis访问结果（trpc::redis::Reply）,在异步接口中我们从Future中获取返回结果（异常情况可能获取不到）。

trpc::redis::Reply对应的八种数据类型:

| 类型 | 类型说明  |   值获取接口   |  
| :-----------| :-------------------:|:----------------: |
| None | 正常访问Redis不会返回此类型的数据|无 | 
| Nil   | 值不存在 |  无 | 
| InValid | 非法类型|  无| 
| String  |  字符串类型|  trpc::redis::Reply::GetString | 
| Status  |  状态类型| trpc::redis::Reply::GetString | 
| Error   | 错误类型，Redis服务端返回的错误信息 |  trpc::redis::Reply::GetString| 
| Integer |整型 |  trpc::redis::Reply::GetInteger | 
| Array  | 数组（嵌套复合类型），数组中的item可能是八种类型中的任意一种|  trpc::redis::Reply::GetArray | 


注意：Redis协议的本身是文本类型的，很多使用者在使用过程中会误认为所有类型都可以通过GetString内容获取reply的返回值。其实不然，不同类型获取值的接口不同。None、Nil、Invalid只有做类型判断，不能获取值，调用trpc::redis::Reply::GetString/GetInteger/GetArray可能会core。另外获取数据的时候推荐带参数高性能接口trpc::redis::Reply::GetArray和trpc::redis::Reply::GetString代替不带参数的接口，减少内部数据的拷贝。

## 4 错误信息
错误信息通过trpc::redis::Reply::GetString获取（Reply的类型为ERROR），得到的错误信息没有错误码，只有文字描述。

# 2 调用下游Redis服务
## 1 生成Redis command
首先需要生成Redis请求命令（command），作为proxy相应接口的传入参数。推荐使用trpc::redis::cmdgen{}.xxxx系列接口生成对应对应的command。

## 2 获取RedisServiceProxy实例
其次需要获取一个下游服务的访问代理对象，类型trpc::redis::RedisServiceProxy。推荐使用TrpcClient::GetProxy方式获取。

### 1 通过配置文件初始化proxy
在配置中设置的proxy相关配置会在服务启动前读入，当调用GetProxy接口获取一个object时程序会根据name去匹配初始化数据，完成proxy对象的初始化。只展示Redis差异部分，配置示例如下：
```yaml
client: #client配置
    service: #调用后端service的配置
        - name: xxxx.xxxx.xxxx.xxxx
          target：xxxx.xxxx.xxxx.xxxx # ip:port 或服务名或域名
          ...
          protocol: redis  #协议名 redis必填项）
          ...
          redis:    #如果不需要鉴权，此项省略。
              password: xxxxx
          ...
```

### 2 通过代码初始化proxy
除了通过配置方式指定，也支持代码方式配置：使用ServiceProxyOption对proxy初始化。只展示Redis差异部分，代码示例如下。
```cpp
auto func = [](ServiceProxyOption* option) {
  option->protocol = "redis";
  option->redis_conf.enable = true        // true需要鉴权，默认false不需要鉴权（非必填项）
  option->redis_conf.password = "xxx";    // 不需要鉴权的场景，省略此行代码（非必填项）
};
// 使用指定的option参数初始化一个proxy
auto proxy = ::trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("redis_service_proxy_name", func);
...
```

## 3 同步调用示例
代码如下：
```cpp
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  trpc::redis::Reply reply;
  auto status = proxy->Command(ctx, &reply, trpc::redis::cmdgen{}.get("trpc_cpp_redis_fiber"));

  if (!status.OK()) {
    // RPC get error,maybe timeout
    std::cout << "Fiber call redis get command fail, error msg:" << status.ErrorMessage() << std::endl;
    gProgramRet = -1;
  } else if (reply.IsError()) {
    // Redis-Server return error
    std::cout << "Fiber call redis get command fail, error msg:" << reply.GetString() << std::endl;
  } else if (reply.IsNil()) {
    // Redis-Server return Nil if not exists this redis-key
    std::cout << "Fiber call redis get command return Nil" << std::endl;
  } else {
    // Success
    std::cout << "Fiber call redis get command success, reply:" << reply.GetString() << std::endl;
  }
```

## 4 异步调用示例
代码如下：
```cpp
  trpc::ClientContextPtr get_ctx = trpc::MakeClientContext(proxy);
  trpc::Latch latch(1);
  auto fut =
      proxy->AsyncCommand(get_ctx, trpc::redis::cmdgen{}.get("trpc_cpp_redis_future"))
          .Then([proxy, &latch](trpc::Future<trpc::redis::Reply>&& get_fut) {
            latch.count_down();

            if (get_fut.IsReady()) {
              auto get_reply = std::get<0>(get_fut.GetValue());
              if (get_reply.IsError()) {
                // Redis-Server return error
                std::cout << "Future Async call redis get command fail,error:" << get_reply.GetString() << std::endl;
                return trpc::MakeExceptionFuture<>(trpc::CommonException(get_reply.GetString().c_str()));
              } else if (get_reply.IsNil()) {
                // Redis-Server return Nil if not exists this redis-key
                std::cout << "Future Async call redis get command return Nil" << std::endl;
                return trpc::MakeExceptionFuture<>(trpc::CommonException("Return Nil"));
              }

              // success
              std::cout << "Future Async call redis get command success, reply:" << get_reply.GetString() << std::endl;
              return trpc::MakeReadyFuture<>();
            }

            // RPC get error,maybe timeout
            std::cout << "Future Async call redis get command fail,error:" << get_fut.GetException().what()
                      << std::endl;
            return trpc::MakeExceptionFuture<>(trpc::CommonException(get_fut.GetException().what()));
          });
  
  // 这里只是为了展示同步等待结果，实际使用无需等待
  latch.wait();
```

# 3 特殊场景及注意事项

## 1 Redis原生批量调用
在一些应用场景中需要从Redis中批量操作，如果用单个的Get/Set性能可能不满足使用需求，可以考虑使用批量接口如Mset、Mget。

示例代码：
```cpp
  // 使用Mset访问Redis
  std::vector<std::pair<std::string, std::string>> key_values;
  key_values.push_back(std::make_pair("trpc_cpp_redis_fiber_key1", "value1"));
  key_values.push_back(std::make_pair("trpc_cpp_redis_fiber_key2", "value2"));
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  trpc::redis::Reply reply;
  auto status = proxy->Command(ctx, &reply, trpc::redis::cmdgen{}.mset(key_values));

  if (!status.OK()) {
    // RPC get error,maybe timeout
    std::cout << "Fiber call redis mset command fail, error msg:" << status.ErrorMessage() << std::endl;
    gProgramRet = -1;
  } else if (reply.IsError()) {
    // Redis-Server return error
    std::cout << "Fiber call redis mset command fail, error msg:" << reply.GetString() << std::endl;
  } else {
    // Success
    std::cout << "Fiber call redis mset command success." << std::endl;
  }


  // 使用Mget访问Redis
  std::vector<std::string> keys;
  keys.push_back("trpc_cpp_redis_fiber_key1");
  keys.push_back("trpc_cpp_redis_fiber_key2");
  trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
  trpc::redis::Reply reply;

  auto status = proxy->Command(ctx, &reply, trpc::redis::cmdgen{}.mget(keys));
  if (!status.OK()) {
    // RPC get error,maybe timeout
    std::cout << "Fiber call redis mget command fail, error msg:" << status.ErrorMessage() << std::endl;
    gProgramRet = -1;
  } else if (reply.IsError()) {
    // Redis-Server return error
    std::cout << "Fiber call redis mget command fail, error msg:" << reply.GetString() << std::endl;
  } else {
    // Success
    std::cout << "Fiber call redis mget command success, reply:" << reply.GetArray().at(0).GetString() << ","
              << reply.GetArray().at(1).GetString() << std::endl;
  }

```
## 2 链接层pipeline
链接层pipeline指的是后端服务回响应的顺序跟接收到请求的顺序一一对应，此时客户端就可以在同一个链接上同时处理多次请求，能做到链接共享，大幅度提升性能。(当然使用链接层pipeline需要
明确后端服务是否支持这种"按序响应"的能力，否则的话禁止使用)

目前使用链接层pipeline 不需要调整代码，只需要调整support_pipeline为true即可。

- 通过配置启用链接层pipeline方式：
```yaml
client:
  service:
    - name: redis_server
      protocol: redis
      xxx
      # For high performance,if the backend Redis Server support the Pipeline mode (responding in the order of requests within the connection)
      # you can use connection-pipeline which config like this:
      support_pipeline: true
```

- 通过代码指定配置项启用链接层pipeline方式：
```cpp
auto func = [](ServiceProxyOption* option) {
  option->protocol = "redis";
  option->redis_conf.enable = true        // true需要鉴权，默认false不需要鉴权（非必填项）
  option->redis_conf.password = "xxx";    // 不需要鉴权的场景，省略此行代码（非必填项）
  
  // 启用链接层pipeline
  option->support_pipeline = true;
};
// 使用指定的option参数初始化一个proxy
auto proxy = ::trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("redis_service_proxy_name", func);
...
```
## 3 选库及自定义鉴权
支持选库和支持Redis 6.0使用username+password鉴权。使用方式：只需在配置中添加，如下所示：
```yaml
client: #client配置
    service: #调用后端service的配置
	...
	redis:
          password: xxxx   #可选 
          user_name: xxxx   #可选 
          db: 2                #可选，选库的dbnum, 默认为0，默认不需要选库时可以删掉此项
```
## 4 一些性能调优经验
可以考虑从以下几个方面入手性能调优：


1）在时延要求高的场景，推荐使用Fiber；

2）使用trpc::redis::RedisServiceProxy推荐的高性能（右值)接口，减少内部数据拷贝；

3）设置合理的连接数（并非越大越好，需要根据业务场景压测）；

4）推荐使用带参数的trpc::redis::Reply::GetArray或者trpc::redis::Reply::GetString获取返回数据，减少内部数据拷贝；

5）在可能的情况下，为了充分利用网络吞吐能力，推荐使用链接层pipeline模式(前提是后端Redis服务也必须支持pipeline能力)访问Redis；

6）在可能的情况下，考虑使用批量接口代替单条数据的访问；

7）热数据本地缓存，数据的过期时间设置随机，防止同一时间大量数据过期现象发生；

8）可以考虑不同的数据存到不同的Redis服务中；

9）锁粒度尽量要小；

10）在不关心Redis命令执行结果的场景中，可以使用单向接口（谨慎选择）。

## 5 tRPC-Cpp Redis 客户端与Redis官方客户端差异
- tRPC-Cpp Redis 客户端暂不支持的命令

| 命令（命令组） |                               说明                                                   | 
| :-----------:| :----------------------------------------------------------------------------------------: |
|    Stream    | Redis Stream 是 Redis 5.0 版本新增加的数据结构, trpc-cpp Redis 客户端暂不支持stream相关命令 |
|    发布订阅   | 暂不支持sub/pub相关命令 |
|    select    | 暂不支持选库功能 |
|    事务      | 暂不支持multi/discard/exec/watch/unwatch等命令|
|    dump      | 不支持dump命令|
|   集群管理  | 不支持集群管理相关命令| 

- tRPC-Cpp Redis 客户端与官方最新客户端的协议差异
Redis官方6.0版本已经发布，对比tRPC-Cpp客户端，有以下差异：

1.鉴权过程支持用户名
老版本的鉴权只需要传密码即可，不需要用户名，6.0支持通过用户名实现更小粒度的访问控制权限。

2.支持的类型更丰富
Redis 6.0除了基本数据类型，扩展出更丰富的类型，tRPC-Cpp目前仅支持基本类型。

| 类型 | tRPC-Cpp Redis客户端 是否支持  |   备注   |  
| :-----------| :-------------------:|:----------------: |
| REDIS_REPLY_STATUS |✓  |基本类型 | 
| REDIS_REPLY_ERROR | ✓ |基本类型 | 
| REDIS_REPLY_INTEGER | ✓  | 基本类型| 
| REDIS_REPLY_NIL | ✓  | 基本类型| 
| REDIS_REPLY_STRING |✓  | 基本类型| 
| REDIS_REPLY_ARRAY |✓ |基本类型 | 
| REDIS_REPLY_DOUBLE | ×| 扩展类型| 
| REDIS_REPLY_BOOL | ×|扩展类型 | 
| REDIS_REPLY_MAP | ×|扩展类型 | 
| REDIS_REPLY_SET | ×|扩展类型 | 
| REDIS_REPLY_PUSH | ×| 扩展类型| 
| REDIS_REPLY_ATTR | ×| 扩展类型| 
| REDIS_REPLY_BIGNUM | ×| 扩展类型| 
| REDIS_REPLY_VERB | ×|扩展类型 | 

3.对SSL的支持
Redis 6.0具备了对SSL的支持能力，默认不启用。在tRPC-Cpp当前版本暂不支持。

# 4 FAQ
在访问Redis遇到问题时，可以先对照下面自查。

## 1 写数据被截断？
问题描述：
调用Status trpc::redis::RedisServiceProxy::Command(const ClientContextPtr& context, Reply* reply, const char* format, ...); 
format串中包含‘\0'会被截断，导致二进制数据写入不全

解决办法：
当foramt字符串中有‘\0‘、’\r'、‘\n‘等字符时使用 “%b" 并指明长度，示例：
Command(ctx_, &rep, "SET %b %b", "trpc", 4, "redis\0redis", 11);
Command(ctx_, &rep, "SET %b %b","trpc",4, "\xAC\xED\x00redis", 8);

所以还是推荐使用 trpc::redis::cmdgen{}.xxxx系列接口生成对应的command，直接把生成的command传入proxy对应接口访问Redis服务。

## 2 Get二进制数据出错？
问题描述：
把struct的二进制转成value,用%b的方式构建set命令存储数据到Redis服务；然后通过get commond访问Redis获取数据，一直返回nil

解决办法：
通过分析发现用户自定义struct存在内存对齐的问题。

## 3 Redis reply GetString core的问题？
参考返回值的描述，建议对Reply先做类型判断再尝试获取值。

## 4 可以自己拼Redis命令吗？
可以，前提是对Redis协议及序列化有基本认识。用户在使用hiredis时，直接传 "get xxx"或者"set xxx"这样的command， hiredis在发送请求给服务端之前会对用户传入的command序列化。RedisServiceProxy::Command/CommandArgv带format参数的函数，tRPC-Cpp提供了对这种用法的兼容，不推荐这种用法，比较容易出错。推荐使用trpc::redis::cmdgen{}.xxxx系列接口生成command传给RedisServiceProxy提供的接口，同时下面这种用法应当禁止：
```cpp 
  // 错误用法示例
  auto get_ctx = trpc::MakeClientContext(proxy);
  trpc::redis::Reply get_reply;
  std::string get_cmd = "get key";  // 禁止这样自己拼字符串
  auto get_status = proxy->Command(get_ctx, &get_reply, std::move(get_cmd));

  if (!mset_status.OK()) {
    std::cout << "redis get command error: " << mset_status.ErrorMessage()
              << std::endl;
    gProgramRet = -1;
  } else {
    std::cout << "redis get command success. reply: " << get_reply << std::endl;
  }

```
## 5 常见的超时错误及解决办法

1) 错误的command服务端无法别
自己拼Redis访问command，一旦出错，服务端无法识别，会一直hang住，不会返回任何数据给客户端，导致等待超时。

2) 链路超时
timeout配置的时间足够长，Redis刚调用就直接报超时错误。日志中记下调用之前和调用之后的时间差，如果这个时间差小于timeout配置的值，可以断定就是链路超时导致的，可尝试禁用链路超时。

3) 数据包比较大
在允许的前提下适当调整timeout的值，同时可以参考"常见的性能调优手段"做一些性能优化。

4) 跨城访问
下游服务的节点分布在多个城市，出现了跨城市访问的情况。建议上下游服务不要跨城部署。


## 6 先执行incr,然后再执行get，通过trpc::redis::Reply::GetInteger()得到的数值与预期不一致？
可能是Redis服务端返回的数据类型是String类型，通过String转Integer发现与预期一致。在取值之前，需要做一下类型判断，碰到类型不匹配的时候，需要业务层做类型转换。

## 7 下游只有一个Redis服务（service target 唯一）, 不同的类型的请求如何使用不同的超时参数？
在业务场景中，只有一个Redis服务。有一部分Redis访问请求，比较重，耗时高；另外一部分请求比较轻量，耗时低。我们希望比较重的那部分请求，timeout设置得高一点；比较轻的那部分请求，timeout设置低一点。有两种解决办法：

1）请求级别设置超时参数：在clientcontext中单独设置timeout；

2）RedisServiceProxy级别设置超时参数：把Redis服务当成两个下游服务来用，配置两个RedisServiceProxy。service name不同，target相同，两个service的timeout不同。获取RedisServiceProxy的时候根据service name来获取不同的Proxy，不同类型的请求用不同的proxy调用，以此类推。

## 8 fiber场景 Redis单条数据较大时（>10M)数据会莫名其妙地core？
排除前面的列举的错误后，也许可以朝着栈溢出的方向考虑，也就是调整fiber栈大小配置：fiber_stack_size

## 9 后端Redis节点比较多的情况下，通过pipeline访问后端时，会OOM？
原因：使用pipeline访问下游时，每个下游节点会创建max_conn_num(默认是64个)个FiberTcpPipelineConnector对象，这个对象比较占用内存，因为使用的proxy 和每个RedisServiceProxy对应的下游比较多，整体下来就是FiberTcpPipelineConnector会比较多，可能导致OOM；
解决方法：pipeline模式下，尝试调小ServiceProxyOption.max_conn_num 个数，比如设置为1即可，或者调小ServiceProxyOption.fiber_pipeline_connector_queue_size大小。
## 10 能否支持自定义命令？
支持，如下：

自定义命令：eco_mget key [key ...] mode
```cpp
std::vector<std::string> keys;
std::string mode;
trpc::redis::Request req;
req.params_.push_back("eco_mget");
req.params_.insert(req.params_.end(), keys.begin(), keys.end());
req.params_.push_back(mode);

proxy->CommandArgv(ctx_, req, &reply);
``` 

自定义命令：router_get cluster database collection

```cpp
std::string cluster;
std::string database;
std::string collection;
proxy->Command(ctx_, &reply, "router_get %s %s %s", cluster.c_str(), database.c_str(), collection.c_str());
```
