
[中文](../zh/redis_client_guide.md)

# Overview

Redis is one of the most popular in-memory databases today, widely used in various businesses. tRPC-Cpp currently only provides support for the Redis client protocol, so in this article, the terms 'Redis call,' 'Redis usage,' and 'Redis development' all refer to accessing downstream Redis services in a client-side manner. In terms of functionality, tRPC-Cpp provides almost all capabilities of the native Redis protocol (with a few exceptions such as sub/pub and select commands).

This article primarily focuses on how to use the tRPC-Cpp framework to call a Redis service. It provides detailed explanations of some specific attributes unique to the Redis protocol, while briefly mentioning common aspects. Before reading this article, please make sure to read it carefully[tRPC-Cpp client_guide](./client_guide.md)。

## Redis protocol

The Redis client communicates with the Redis server using the RESP (Redis Serialization Protocol) protocol. Although this protocol is designed specifically for Redis, it can also be used in other client-server software projects. The Redis protocol is a text-based protocol, where commands or data sent by the client and server are terminated with `\r\n`(CRLF). Some developers mistakenly believe that the Redis protocol is space-separated and prefer to manually construct Redis access commands when using the tRPC-Cpp client. It is recommended to use the `trpc::redis::cmdgen{}.xxxx` series of interfaces to generate corresponding [command](https://redis.io/commands) when not familiar with the Redis protocol itself. Then, simply pass the generated command to the corresponding proxy interface to access the Redis server."

## Interface

Currently, Redis access is initiated through the trpc::redis::RedisServiceProxy class, which provides two types of interfaces: synchronous calls and asynchronous calls

### Synchronous interface

| Class          |  Interface |  Function |Parameter |Return value|Note|
| :-------------------| :-------------------------------------------------------------------------------:|:---------------------|:------------------------|:-----------|:---------------: |
|RedisServiceProxy | Command(const ClientContextPtr& context, Reply* reply, const std::string& cmd) |  returning the result of executing a Redis command|context  <br> reply  result of executing a Redis command <br> cmd Redis command | Status | |
|RedisServiceProxy | Command(const ClientContextPtr& context, Reply* reply, std::string&& cmd) | returning the result of executing a Redis command|context  <br> reply  result of executing a Redis command <br> cmd Redis command  | Status |   |
|RedisServiceProxy | Command(const ClientContextPtr& context, Reply&amp; reply, const char* format, ...) | returning the result of executing a Redis command| context <br> reply  result of executing a Redis command <br> generating Redis commands using the format method | Status |prone to errors |
|RedisServiceProxy | CommandArgv(const ClientContextPtr& context, const Request& req, Reply* reply) | returning the result of executing a Redis command| context  <br> req RedisRequest <br> reply  result of executing a Redis command   | Status |   |
|RedisServiceProxy | CommandArgv(const ClientContextPtr& context, Request&& req, Reply* reply) | returning the result of executing a Redis command| context  <br> req RedisRequest <br> reply  result of executing a Redis command   |  Status |  |
|RedisServiceProxy | Command(const ClientContextPtr& context, const std::string& cmd) | executing Redis commands without returning results| context <br> cmd Redis Command |  Status | OneWay Call，use with caution |
|RedisServiceProxy | Command(const ClientContextPtr& context, std::string&& cmd) | executing Redis commands without returning results| context  <br> cmd Redis Command |  Status |OneWay Call，use with caution |

### Asnchronous interface

| Class          |  Interface |  Function |Parameter |Return value|Note|
| :-------------------| :-------------------------------------------------------------------------------:|:---------------------|:------------------------|:-----------|:---------------: |
|RedisServiceProxy | AsyncCommand(const ClientContextPtr& context, const std::string& cmd) |  returning the result of executing a Redis command|context  <br>  cmd Redis Command | Future&lt;Reply> | |
|RedisServiceProxy | AsyncCommand(const ClientContextPtr& context, std::string&& cmd) | returning the result of executing a Redis command|context  <br>  cmd Redis Command  | Future&lt;Reply> |  |
|RedisServiceProxy | AsyncCommand(const ClientContextPtr& context, const char* format, ...) | returning the result of executing a Redis command| context  <br> format generating Redis commands using the format method| Future&lt;Reply> |prone to errors |
|RedisServiceProxy | AsyncCommandArgv(const ClientContextPtr& context, const Request& req) | returning the result of executing a Redis command| context  <br> req RedisRequest    | Future&lt;Reply> |   |
|RedisServiceProxy | AsyncCommandArgv(const ClientContextPtr& context, Request&& req) | returning the result of executing a Redis command| context  <br> req RedisRequest |  Future&lt;Reply> |  |

For both synchronous and asynchronous interfaces, it is recommended to use the corresponding right-value interfaces for performance reasons. Left-value interfaces should only be used in scenarios where cmd or req calls need to be used afterwards

## Return value

The return value type for synchronous interfaces is Status, and the Redis access result is obtained through the reply output parameter (trpc::redis::Reply). In asynchronous interfaces, we obtain the return result from the Future (which may not be available in exceptional cases).

The eight data types corresponding to trpc::redis::Reply are:

| Type | Type Description | Value Retrieval Interface |
| :-----------| :-------------------:|:----------------: |
| None | This type of data is not returned during normal Redis access | N/A |
| Nil | Value does not exist | N/A |
| InValid | Invalid type | N/A |
| String | String type | trpc::redis::Reply::GetString |
| Status | Status type | trpc::redis::Reply::GetString |
| Error | Error type, error message returned by Redis server | trpc::redis::Reply::GetString |
| Integer | Integer type | trpc::redis::Reply::GetInteger |
| Array | Array (nested composite type), items in the array can be any of the eight types | trpc::redis::Reply::GetArray |

Note: The Redis protocol itself is text-based, and many users mistakenly assume that all types can be obtained through GetString to retrieve the return value of the reply. However, this is not the case, as different types have different interfaces for value retrieval. None, Nil, and Invalid are only used for type checking and cannot retrieve values. Calling trpc::redis::Reply::GetString/GetInteger/GetArray may result in a core dump. Additionally, when retrieving data, it is recommended to use the parameterized high-performance interfaces trpc::redis::Reply::GetArray and trpc::redis::Reply::GetString instead of the parameterless interfaces to reduce internal data copying.

## Error Message

Error messages can be obtained through trpc::redis::Reply::GetString (Reply type is ERROR). The error messages obtained do not include error codes, only textual descriptions.

# Call Redis

## Generate Redis command

First, it is necessary to generate the Redis request command (command) as an input parameter for the corresponding proxy interface. It is recommended to use the `trpc::redis::cmdgen{}.xxxx`series of interfaces to generate the corresponding command.

## Get RedisServiceProxy

Next, you need to obtain an access proxy object for the downstream service, of type trpc::redis::RedisServiceProxy. It is recommended to use the TrpcClient::GetProxy method to obtain it

### Init proxy by config

The proxy-related configurations set in the configuration will be read before the service starts. When calling the GetProxy interface to obtain an object, the program will match the initialization data based on the name and complete the initialization of the proxy object. An example of the configuration is as follows:

```yaml
client:
    service:
        - name: xxxx.xxxx.xxxx.xxxx
          target：xxxx.xxxx.xxxx.xxxx
          ...
          protocol: redis）
          ...
          redis:    # this section can be omitted if authentication is not required
              password: xxxxx
          ...
```

### Init proxy by code

In addition to specifying through configuration, it also supports configuring via code: initializing the proxy using ServiceProxyOption. Here is an example of the code:

```cpp
auto func = [](ServiceProxyOption* option) {
  option->protocol = "redis";
  option->redis_conf.enable = true        // "true" requires authentication, "false" is the default and does not require authentication (optional).
  option->redis_conf.password = "xxx";    
};

auto proxy = ::trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("redis_service_proxy_name", func);
...
```

## Synchronous call code

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

## Asynchronous call code

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
  
  // This is just to demonstrate synchronous waiting for the result. In actual usage, there is no need to wait.
  latch.wait();
```

# Special Scenarios and Considerations

## Redis Native Batch Calls

In some application scenarios, batch operations are required on Redis. If using individual Get/Set operations may not meet the performance requirements, consider using batch interfaces such as Mset and Mget.

```cpp
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

## Connection-level Pipeline

Connection-level pipeline refers to the correspondence between the order of responses from the backend service and the order in which the requests were received. In this case, the client can process multiple requests simultaneously on the same connection, achieving connection sharing and significantly improving performance. (Of course, using connection-level pipeline requires confirming whether the backend service supports this 'in-order response' capability. Otherwise, it is prohibited to use.)

Currently, using connection-level pipeline does not require code adjustments. You only need to set support_pipeline to true.

- use connection-level pipeline by config：

  ```yaml
  client:
    service:
      - name: redis_server
        protocol: redis
        # xxx
        # For high performance,if the backend Redis Server support the Pipeline mode (responding in the order of requests within the connection)
        # you can use connection-pipeline which config like this:
        support_pipeline: true
  ```

- use connection-level pipeline by code

  ```cpp
  auto func = [](ServiceProxyOption* option) {
    option->protocol = "redis";
    option->redis_conf.enable = true        
    option->redis_conf.password = "xxx";    
  
    // enable connection-level
    option->support_pipeline = true;
  };
  auto proxy = ::trpc::GetTrpcClient()->GetProxy<trpc::redis::RedisServiceProxy>("redis_service_proxy_name", func);
  ...
  ```

## Select DB and Auth

Support for database selection and Redis 6.0 authentication using username+password. To use this feature, simply add it to the configuration as shown below:

```yaml
client:
    service:
  # ...
  redis:
          password: xxxx
          user_name: xxxx
          db: 2                #Optional, the dbnum for database selection, default is 0. If no database selection is needed, you can remove this item
```

## Some performance optimization tips to consider

- For scenarios with high latency requirements, it is recommended to use Fiber.

- Use the high-performance (rvalue) interfaces recommended by trpc::redis::RedisServiceProxy to reduce internal data copying.

- Set a reasonable number of connections (more is not always better, it should be based on business scenario load testing).

- It is recommended to use trpc::redis::Reply::GetArray or trpc::redis::Reply::GetString with parameters to retrieve the returned data, reducing internal data copying.

- Whenever possible, use connection-level pipeline mode (provided the backend Redis service supports pipeline capability) to fully utilize network throughput.

- Whenever possible, consider using batch interfaces instead of accessing individual data items.

- Implement local caching for hot data, with randomized expiration time to prevent a large number of data expirations occurring simultaneously.

- Consider storing different data in different Redis services.

- Keep the lock granularity as small as possible.

- In scenarios where the result of Redis command execution is not important, consider using one-way interfaces (choose carefully).

## Differences between tRPC-Cpp Redis Client and Redis Official Client

### Commands not supported by tRPC-Cpp Redis Client

| Command (Command Group) |                               Explanation                                                   |
| :-----------:| :----------------------------------------------------------------------------------------: |
|    Stream    | Redis Stream is a data structure introduced in Redis 5.0, and tRPC-Cpp Redis Client currently does not support stream-related commands. |
|    Publish/Subscribe   | Publish/Subscribe commands (sub/pub) are not currently supported. |
|    Select    | Database selection functionality (select command) is not currently supported. |
|    Transactions      | Commands such as multi/discard/exec/watch/unwatch for transactions are not currently supported. |
|    Dump      | Dump command is not supported. |
|   Cluster Management  | Cluster management-related commands are not supported. |

### Differences between tRPC-Cpp Redis Client and the Latest Official Client

Redis Official Client version 6.0 has been released, and in comparison to the tRPC-Cpp client, there are the following differences:

1. Authentication Process Supports Username

    In older versions, authentication only required a password and did not require a username. However, in version 6.0, authentication supports usernames, allowing for finer-grained access control permissions.

2. Support for More Data Types

    Redis 6.0 introduces a wider range of data types in addition to the basic data types. However, tRPC-Cpp currently only supports basic types.

    | Type | tRPC-Cpp Redis Client Support  |   Note   |  
    | :-----------| :-------------------:|:----------------: |
    | REDIS_REPLY_STATUS |✓  |Basic Type |
    | REDIS_REPLY_ERROR | ✓ |Basic Type |
    | REDIS_REPLY_INTEGER | ✓  | Basic Type|
    | REDIS_REPLY_NIL | ✓  | Basic Type|
    | REDIS_REPLY_STRING |✓  | Basic Type|
    | REDIS_REPLY_ARRAY |✓ |Basic Type |
    | REDIS_REPLY_DOUBLE | ×| Extend Type|
    | REDIS_REPLY_BOOL | ×|Extend Type |
    | REDIS_REPLY_MAP | ×|Extend Type |
    | REDIS_REPLY_SET | ×|Extend Type |
    | REDIS_REPLY_PUSH | ×| Extend Type|
    | REDIS_REPLY_ATTR | ×| Extend Type|
    | REDIS_REPLY_BIGNUM | ×| Extend Type|
    | REDIS_REPLY_VERB | ×|Extend Type |

3. Support SSL

    Redis 6.0 has the capability to support SSL, but it is not enabled by default. However, the current version of tRPC-Cpp does not support SSL.

# FAQ

## Data truncation issue?

- Problem description:

  > When calling the function `Status trpc::redis::RedisServiceProxy::Command(const ClientContextPtr& context, Reply* reply, const char* format, ...);`, 
  > if the format string contains '\0', it will be truncated, resulting in incomplete writing of binary data.

- Solution:

  > When the format string contains characters such as '\0', '\r', '\n', etc., use `%b` and specify the length. For example:
  > - Command(ctx_, &rep, "SET %b %b", "trpc", 4, "redis\0redis", 11);
  > - Command(ctx_, &rep, "SET %b %b", "trpc", 4, "\xAC\xED\x00redis", 8);

Therefore, it is recommended to use the trpc::redis::cmdgen{}.xxxx series of interfaces to generate the corresponding command. Then, pass the generated command directly to the proxy interface to access the Redis service.

## Error retrieving binary data with Get?

- Problem description:

  > When converting binary data from a struct to a value and storing it in Redis using the `SET` command with the `%b` format, retrieving the data with the `GET` command in Redis always returns `nil`.

- Solution:

  > Upon analysis, it was found that there is a memory alignment issue with the user-defined struct.

## Redis reply GetString core?

Based on the description of the return value, it is recommended to perform type checking on the Reply before attempting to retrieve the value.

## Can I construct Redis commands myself?

Yes, it is possible, provided that you have a basic understanding of the Redis protocol and serialization. When using hiredis, you can directly pass commands like "get xxx" or "set xxx". Hiredis serializes the command before sending the request to the server. The RedisServiceProxy::Command/CommandArgv functions in tRPC-Cpp are compatible with this usage, but it is not recommended as it can be prone to errors. It is recommended to use the trpc::redis::cmdgen{}.xxxx series of interfaces to generate commands and pass them to the interfaces provided by RedisServiceProxy. Additionally, the following usage should be avoided:

```cpp
  // wrong example
  auto get_ctx = trpc::MakeClientContext(proxy);
  trpc::redis::Reply get_reply;
  std::string get_cmd = "get key";  // forbidden
  auto get_status = proxy->Command(get_ctx, &get_reply, std::move(get_cmd));

  if (!mset_status.OK()) {
    std::cout << "redis get command error: " << mset_status.ErrorMessage()
              << std::endl;
    gProgramRet = -1;
  } else {
    std::cout << "redis get command success. reply: " << get_reply << std::endl;
  }

```

## Timeout errors and solutions

- Incorrect command causing server inability to respond

  When constructing Redis commands manually, if an error occurs, the server may not be able to recognize it and will hang indefinitely without returning any data to the client, resulting in a timeout. **It is recommended to use the provided interfaces to construct commands** and avoid manual command construction.

- Full-link timeout

  If the timeout configuration is set to a relatively short duration, Redis may report a timeout error immediately after the call. To identify if it is a link timeout issue, log the time difference before and after the call. If this time difference is smaller than the configured timeout value, it can be concluded that it is a link timeout issue. In such cases, disabling the link timeout can be attempted.

- Large data packets

  If dealing with large data packets, adjust the timeout value appropriately within the allowed limits. Additionally, refer to "Common Performance Optimization Techniques" for further performance optimization.

- Cross-city access

  If the downstream service nodes are distributed across multiple cities, cross-city access may occur. It is recommended to avoid deploying upstream and downstream services across different cities.

## When executing INCR followed by GET, if the value obtained using trpc::redis::Reply::GetInteger() is inconsistent with the expected result?

It is possible that the data type returned by the Redis server is a string. Converting the string to an integer may yield the expected result. Before retrieving the value, it is necessary to perform type checking. If a type mismatch occurs, the business layer needs to handle the type conversion.

## How to use different timeout parameters for different types of requests when there is only one downstream Redis service (with a unique service target)?

In the given business scenario, there is only one Redis service. Some Redis access requests are heavy and time-consuming, while others are lightweight and quick. We want to set a higher timeout for the heavy requests and a lower timeout for the lightweight requests. There are two possible solutions:

- Request-level timeout parameter

  Set the timeout individually in the client context for each request;

- RedisServiceProxy-level timeout parameter

  Treat the Redis service as two separate downstream services by configuring two RedisServiceProxy instances. The service names will be different, but the target will remain the same. Set different timeouts for the two services. When obtaining the RedisServiceProxy, use the service name to retrieve the appropriate proxy. Invoke different types of requests using the corresponding proxy, and so on.

## When fiber, if individual data entries are relatively large (>10MB), the program may unexpectedly crash (core dump)？

In this case, adjusting the fiber stack size configuration (fiber_stack_size) could be a potential solution.

## When there are a large number of backend Redis nodes, accessing them through a pipeline can lead to out-of-memory (OOM) issues？

- Reason:
  > When accessing backend nodes through a pipeline, each backend node creates max_conn_num (default is 64) FiberTcpPipelineConnector objects. These objects consume a significant amount of memory. Since there are multiple downstream nodes corresponding to each proxy and the overall number of FiberTcpPipelineConnectors can be high, it can lead to OOM issues.

- Solution:
  > In pipeline mode, you can try reducing the number of max_conn_num in ServiceProxyOption. For example, setting it to 1 or reducing the fiber_pipeline_connector_queue_size in ServiceProxyOption can help mitigate the OOM problem.

## Custom commands？

- Custom command：eco_mget key [key ...] mode

  ```cpp
  std::vector<std::string> keys;
  std::string mode;
  trpc::redis::Request req;
  req.params_.push_back("eco_mget");
  req.params_.insert(req.params_.end(), keys.begin(), keys.end());
  req.params_.push_back(mode);
  
  proxy->CommandArgv(ctx_, req, &reply);
  ```

- Custom command：router_get cluster database collection

  ```cpp
  std::string cluster;
  std::string database;
  std::string collection;
  proxy->Command(ctx_, &reply, "router_get %s %s %s", cluster.c_str(), database.c_str(), collection.c_str());
  ```
