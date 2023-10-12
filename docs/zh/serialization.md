[English](../en/serialization.md)

# 前言

本文介绍 tRPC-Cpp 框架（下面简称 tRPC）消息序列化特性，开发者可以了解到如下内容：

* 如何使用 JSON 作为消息类型。
* 如何使用二进制数据作为消息类型。

tRPC-Cpp 框架默认使用 `Protobuf Message` 作为请求和响应消息类型，同时也支持使用 `trpc` 协议传输 JSON 和二进制数据消息。也就说，可以使用 JSON 和二进制数据作为请求消息和响应消息类型。

# 如何使用 JSON 作为消息类型

示例：[trpc_json](../../examples/features/trpc_json)

## 基本步骤

```mermaid
graph LR
    A[Client] -->|1 Request-JSON | B[Server]
    B --> |2 Response-JSON| A
```

框架当前支持使用 `rapidjson::Document` 作为请求消息和响应消息类型。
上图中，Client 和 Server 使用 JSON 作为请求消息和响应消息，我们在编程的时候使用 rapidjson::Document 作为请求消息和响应消息类型。

我们需要关注这些环节：

1. 在服务端开发新的 service 来处理 rapidjson::Document 类型的请求消息，并回复 rapidjson::Document 类型的响应。
2. 在客户端访问 1 中的 service 时，使用 rapidjson::Document 作为请求消息和响应消息类型。

基本的步骤：

1. 开发 service 来处理 rapidjson::Document 类型的消息。
2. 使用 `trpc` 协议注册 service。

## 实现过程

### 开发 service 来处理 `rapidjson::Document` 类型的消息

框架是通过注册 Service 的方式对外提供服务；所以，需要开发一个 ServiceImpl 来处理 rapidjson::Document 类型的消息，然后回复响应。

框架提供了 `RpcServiceImpl` 接口。我们只需要做两件事：

1. 实现一个 rapidjson::Document 消息处理方法，这里添加具体请求的处理过程。
2. 注册请求路由，这个过程一般放在构造方法中。

   ```cpp
   class DemoServiceImpl : public ::trpc::RpcServiceImpl {
    public:
     DemoServiceImpl() {
       auto handler = new ::trpc::RpcMethodHandler<rapidjson::Document, rapidjson::Document>(
           std::bind(&DemoServiceImpl::JsonSayHello, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
       AddRpcServiceMethod(new ::trpc::RpcServiceMethod("JsonSayHello", ::trpc::MethodType::UNARY, handler));
     }
   
     ::trpc::Status JsonSayHello(const ::trpc::ServerContextPtr& context, const rapidjson::Document* request, rapidjson::Document* reply) {
       for (rapidjson::Value::ConstMemberIterator iter = request->MemberBegin(); iter != request->MemberEnd(); ++iter) {
         TRPC_FMT_INFO("json name: {}, value: {}", iter->name.GetString(), iter->value.GetInt());
       }
       reply->CopyFrom(*request, const_cast<rapidjson::Document*>(request)->GetAllocator());
       return ::trpc::kSuccStatus;
     }
   };
   ```

### 使用 `trpc` 协议注册 service

注册 `DemoServiceImpl` 和注册 tRPC-Cpp 服务一模一样。

**提示：配置文件中 protocol 配置项使用 `trpc` 协议。**

* 示例代码

  ```cpp
  class DemoServer : public ::trpc::TrpcApp {
   public:
    int Initialize() override {
      const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
      // Set the service name, which must be the same as the value of the `server:service:name` configuration item
      // in the framework configuration file, otherwise the framework cannot receive requests normally
      std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "demo_service");
  
      TRPC_FMT_INFO("service name:{}", service_name);
  
      RegisterService(service_name, std::make_shared<DemoServiceImpl>());
  
      return 0;
    }
  
    void Destroy() override {}
  };
  ```

* 服务配置如下：

  ```yaml
  # @file: trpc_cpp.yaml
  #...
  server:
    app: test
    server: helloworld
    admin_port: 18888
    admin_ip: 0.0.0.0
    service:
      - name: trpc.test.helloworld.demo_service
        protocol: trpc
        network: tcp
        ip: 0.0.0.0
        port: 12349
  #...
  ```

* 服务调用

  使用 tRPC-Cpp 客户端访问 DemoServiceImpl 也很简单，和访问 tRPC 服务一样。创建 `RpcServiceProxy` 对象 proxy，调用 `UnaryInvoke` 或者 `AsyncUnaryInvoke` 方法。

  ```cpp
  int Call() {
    ::trpc::ServiceProxyOption option;
  
    option.name = FLAGS_target;
    option.codec_name = "trpc";
    option.network = "tcp";
    option.conn_type = "long";
    // ...
    
    auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::RpcServiceProxy>(FLAGS_target, option);
  
    std::string req_json_str = "{\"age\":18,\"height\":180}";
    rapidjson::Document hello_req;
    hello_req.Parse(req_json_str.c_str());
  
    if (hello_req.HasParseError()) {
      std::cout << "json parse error: " << hello_req.GetParseError()
                << ", msg: " << rapidjson::GetParseError_En(hello_req.GetParseError()) << std::endl;
      return -1;
    }
  
    ::trpc::ClientContextPtr context = ::trpc::MakeClientContext(proxy);
    context->SetTimeout(1000);
    context->SetFuncName("JsonSayHello");
  
    rapidjson::Document hello_rsp;
    ::trpc::Status status = proxy->UnaryInvoke<rapidjson::Document, rapidjson::Document>(context, hello_req, &hello_rsp);
  
    if (!status.OK()) {
      std::cout << "invoke error: " << status.ErrorMessage() << std::endl;
      return -1;
    }
  
    for (rapidjson::Value::ConstMemberIterator iter = hello_rsp.MemberBegin(); iter != hello_rsp.MemberEnd(); ++iter) {
      std::cout << "json name: " << iter->name.GetString() << ", value: " << iter->value.GetInt() << std::endl;
    }
  
    return 0;
  }
  ```

# 如何使用二进制数据作为消息类型

示例：[trpc_binary_data](../../examples/features/trpc_noop)

```mermaid
graph LR
    A[Client] -->|1 Request-BinaryData| B[Server]
    B --> |2 Response-BinaryData| A
```

框架当前支持使用 `std::string` 或者 `::trpc::NoncontiguousBuffer` 作为请求消息和响应消息类型。
上图中，Client 和 Server 使用二进制数据作为请求消息和响应消息，我们在编程的时候使用 `std::string` 或者 `::trpc::NoncontiguousBuffer` 作为请求消息和响应消息类型。

具体方法和步骤与使用`rapidjson::Document`作为请求消息和响应消息类型的实现过程一致。我们看下示例代码关键片段，具体细节可以参考示例代码。

```cpp
class DemoServiceImpl : public ::trpc::RpcServiceImpl {
 public:
  DemoServiceImpl() {
    auto handler1 = new ::trpc::RpcMethodHandler<std::string, std::string>(
        std::bind(&DemoServiceImpl::NoopSayHello1, this, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3));
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod("NoopSayHello1", ::trpc::MethodType::UNARY, handler1));

    auto handler2 = new ::trpc::RpcMethodHandler<::trpc::NoncontiguousBuffer, ::trpc::NoncontiguousBuffer>(
        std::bind(&DemoServiceImpl::NoopSayHello2, this, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3));
    AddRpcServiceMethod(new ::trpc::RpcServiceMethod("NoopSayHello2", ::trpc::MethodType::UNARY, handler2));
  }

  // std::string is used as the request and the response.
  ::trpc::Status NoopSayHello1(const ::trpc::ServerContextPtr& context, const std::string* request, std::string* reply) {
    *reply = *request;
    return ::trpc::kSuccStatus;
  }

  // ::trpc::NoncontiguousBuffer is used as the request and the response.
  ::trpc::Status NoopSayHello2(const ::trpc::ServerContextPtr& context, const ::trpc::NoncontiguousBuffer* request, ::trpc::NoncontiguousBuffer* reply) {
    *reply = *request;
    return ::trpc::kSuccStatus;
  }
};
```
