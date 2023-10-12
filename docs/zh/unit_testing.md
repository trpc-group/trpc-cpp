[English](../en/unit_testing.md)

# 前言

为了方便对用到网络调用的逻辑代码进行单元测试，一种常用的方式是进行网络 mock，因此框架提供了基于[googletest(googlemock)](https://github.com/google/googletest)的测试方法。

接下来我们就如何编写服务端及客户端的单元测试进行说明。

# 服务端

就使用方式而言，可分为同步回包及异步回包方式。

## 同步回包场景下的单测编写

此场景下不涉及网络调用，因此无需对网络进行 mock。故单测编写比较简单，示例如下：

```cpp
TEST(GreeterServiceTest, SayHelloOK) {
  auto server_context = ::trpc::MakeRefCounted<::trpc::ServerContext>();
  ::trpc::test::unittest::HelloRequest request;
  request.set_msg("trpc");
  ::trpc::test::unittest::HelloReply reply;
  GreeterServiceImpl test_service;
  auto status = test_service.SayHello(server_context, &request, &reply);
  ASSERT_TRUE(status.OK());
  ASSERT_EQ(reply.msg(), "Hello, trpc");
}
```

完整示例请参考example的[GreeterServiceTest_SayHelloOK](/examples/helloworld/greeter_service_test.cc#L32)示例。

## 异步回包场景下的单测编写

异步回包场景相比同步回包场景，需要用户在程序中主动调用`context->SendUnaryResponse`进行回包。此场景涉及网络调用，故单测中需要对网络进行 mock。

为了方便写单测，框架提供了针对这种测试场景的[MockServerTransport](/trpc/server/testing/server_testing.h#L44)类。

使用时先调用 `Service::SetServerTransport` 接口将 transport 设置为 MockServerTransport，然后通过 gmock 的 EXPECT_CALL 语法来对网络调用进行 mock，伪代码如下所示：

```cpp
#include "trpc/server/testing/server_testing.h"

TEST(GreeterServiceTest, SayHelloOK) {
  auto transport= std::make_unique<::trpc::testing::MockServerTransport>();
  GreeterServiceImpl test_service;
  test_service.SetServerTransport(transport.get());

  ::trpc::Latch latch(1);
  EXPECT_CALL(*transport, SendMsg(::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Invoke([&latch](::trpc::ServerContextPtr& context, ::trpc::NoncontiguousBuffer&& buffer) -> int {
        EXPECT_TRUE(context->GetStatus().OK());
        EXPECT_TRUE(!buffer.Empty());
        latch.count_down();
        return 0;
      }));

  ::trpc::TrpcServerCodec codec;
  auto server_context = ::trpc::testing::MakeServerContext(&test_service, &codec);
          EXPECT_TRUE(server_context->GetStatus().OK());

  ::trpc::test::unittest::HelloRequest request;
  ::trpc::test::unittest::HelloReply reply;
  request.set_msg("trpc");
  test_service.SayHello(server_context, &request, &reply);
  latch.wait();
}
```

完整示例见 unittest 中的[GreeterServiceTestFixture_SayHelloOK](/examples/unittest/server/async/greeter_service_test.cc#L55)。

说明：

在调用 `SendUnaryResponse` 进行异步回包时，要求对 ServerContext 对象的一些成员进行初始化，然后才能使用。

因此为了方便测试代码编写，框架提供了[ServerContextPtr MakeServerContext(Service* service, ServerCodec* codec)](/trpc/server/testing/server_testing.h#L27)接口来生成测试用的ServerContext。

# 客户端

客户端调用分为同步及异步调用两种方式，两种情况都需要对网络进行 mock，mock方法也类似。就具体调用方式而言，主要分为两类：

1. 直接使用RPC接口进行调用，如 tRPC 协议
2. 使用HttpServiceProxy、RedisServiceProxy等ServiceProxy子类进行调用

下面对这两种场景下的单测编写进行说明。

## 使用RPC接口调用场景下的单测编写

针对这种场景，框架提供了 mock 类，用户可以基于 mock 类，对 RPC 接口进行 mock 来编写单测。完整的代码示例见[unittest/client](/examples/unittest/client/greeter_client_test.cc)。

### mock 类生成

以 tRPC 协议为例，框架支持通过 protobuf 文件生成对应的 mock 类，然后用户使用 mock 类进行编码。

比如有如下的 protobuf 文件`helloworld.proto`

```protobuf
syntax = "proto3";
package trpc.test.unittest;

service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {}
}
message HelloRequest { 
  string msg = 1;
}
message HelloReply {
  string msg = 1;
}
```

通过在 trpc_proto_library 中添加`generate_new_mock_code = true`，即可开启 mock 类代码生成：

```bzl
# 此处为BUILD文件的trpc_proto_library部分
trpc_proto_library(
    name = "helloworld_proto",
    srcs = ["helloworld.proto"],
    visibility = [
     "//visibility:public",
    ],
    use_trpc_plugin=True,
    rootpath="@trpc_cpp",
    generate_new_mock_code = True, #此处即为启用生成Mock类的选项
)
```

编译之后，将生成包括Mock实现类的头文件 `helloworld.trpc.pb.mock.h`, 其中 `MockGreeterServiceProxy` 类可用于客户端的 mock
测试：

```cpp
class MockGreeterServiceProxy: public GreeterServiceProxy {
public:
  MOCK_METHOD(::trpc::Status, SayHello, (const ::trpc::ClientContextPtr&, const ::trpc::test::unittest::HelloRequest&, ::trpc::test::unittest::HelloReply*));
  MOCK_METHOD(::trpc::Future<::trpc::test::unittest::HelloReply>, AsyncSayHello, (const ::trpc::ClientContextPtr&, const ::trpc::test::unittest::HelloRequest&));
  MOCK_METHOD(::trpc::Status, SayHello, (const ::trpc::ClientContextPtr&, const ::trpc::test::unittest::HelloRequest&));
};
```

在生成 mock 类后，我们就可以使用 mock 类来编写单元测试。

### 使用mock类进行单测编写

假设客户端调用逻辑的代码段如下：

```cpp
using GreeterServiceProxyPtr = std::shared_ptr<::trpc::test::unittest::GreeterServiceProxy>();

// 同步调用
bool SayHello(GreeterServiceProxyPtr& proxy) {
  auto client_context = MakeClientContext(proxy);
  ::trpc::test::unittest::HelloRequest req;
  ::trpc::test::unittest::HelloReply rep;
  req.set_msg("trpc");
  auto status = proxy->SayHello(client_context, req, &rep);
  if (status.OK()) {
    // do something when success
    // ...
    return true;
  } else {
    // do something when fail
    // ...
    return false;
  }
}

// 异步调用
::trpc::Future<> AsyncSayHello(GreeterServiceProxyPtr& proxy) {
  auto client_context = MakeClientContext(proxy);
  ::trpc::test::unittest::HelloRequest req;
  req.set_msg("trpc");
  return proxy->AsyncSayHello(client_context, req).Then(
      [](::trpc::Future<::trpc::test::unittest::HelloReply>&& fut){
        if (fut.IsReady()) {
          // do something when success
          // ...
          return ::trpc::MakeReadyFuture<>();
        } else {
          // do something when fail
          // ...
          return ::trpc::MakeExceptionFuture<>(::trpc::CommonException("invoke fail"));
        }
      });
}
```

这时我们使用 `helloworld.trpc.pb.mock.h` 中生成 `MockGreeterServiceProxy` 对象来对 RPC 接口进行 mock，伪代码如下：

```cpp
using ::trpc::test::unittest;
using MockGreeterServiceProxyPtr = std::shared_ptr<MockGreeterServiceProxy>;

class TestFixture : public ::testing::Test {
 public:
  static void SetUpTestCase() { ... }

  static void TearDownTestCase() { ... }

  void SetUp() override {
    mock_service_proxy_ = ::trpc::GetTrpcClient()->GetProxy<MockGreeterServiceProxy>("mock_proxy");
  }

  void TearDown() override {}

 protected:
  MockGreeterServiceProxyPtr mock_service_proxy_;
};
MockGreeterServiceProxyPtr TestFixture::mock_service_proxy_ = nullptr;

TEST_F(TestFixture, SayHelloSuccess) {
  EXPECT_CALL(*mock_service_proxy_, SayHello(::testing::_, ::testing::_, ::testing::_)).Times(1)
      .WillOnce(::testing::Return(::trpc::kSuccStatus));
  EXPECT_TRUE(SayHello(mock_service_proxy_));
}

TEST_F(TestFixture, AsyncSayHelloSuccess) {
  ::trpc::test::helloworld::HelloReply rep;
  rep.set_msg("Hello");
  EXPECT_CALL(*mock_service_proxy_, AsyncSayHello(::testing::_, ::testing::_)).Times(1)
      .WillOnce(::testing::Return(::testing::ByMove(::trpc::MakeReadyFuture<::trpc::test::helloworld::HelloReply>(std::move(rep)))));

  trpc::Latch latch(1);
  AsyncSayHello(mock_service_proxy_).Then([&latch](::trpc::Future<>&& fut){
    EXPECT_TRUE(fut.IsReady());
    latch.count_down();
    return ::trpc::MakeReadyFuture<>();
  });

  latch.wait();
}
```

## 使用 HttpServiceProxy、RedisServiceProxy 等 ServiceProxy 子类进行调用场景下的单测编写

针对这种场景，可以通过重载基类 ServiceProxy 的如下虚接口来进行网络 mock:

```cpp
/// @brief Synchronous call interface oriented to transport, it will be call by UnaryInvoke.
virtual void UnaryTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp);

/// @brief Asynchronous call interface oriented to transport, it will be call by AsyncUnaryInvoke.
virtual Future<ProtocolPtr> AsyncUnaryTransportInvoke(const ClientContextPtr& context,
                                                        const ProtocolPtr& req_protocol);

/// @brief One-way call interface oriented to transport, it will be call by OnewayInvoke.
virtual void OnewayTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req);
```

以HttpServiceProxy同步调用方式为例(异步调用方式方法类似，不再进行说明)，对下面的代码进行测试:

```cpp
using HttpServiceProxyPtr = std::shared_ptr<::trpc::http::HttpServiceProxy>;

Status GetContent(HttpServiceProxyPtr& proxy, const std::string& url, rapidjson::Document* json) {
  auto client_context = ::trpc::MakeClientContext(proxy);
  return proxy->Get(context, url, json);
}
```

则步骤如下：

1. 实现 HttpServiceProxy mock 类 MockHttpServiceProxy

   ```cpp
   class MockHttpServiceProxy : public ::trpc::HttpServiceProxy {
    public:
     MOCK_METHOD(void, UnaryTransportInvoke, (const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp),
                 (override));
   
     MOCK_METHOD(Future<ProtocolPtr>, AsyncUnaryTransportInvoke, (const ClientContextPtr& context, const ProtocolPtr& req_protocol), (override));
   };
   ```
  
2. 使用 MockHttpServiceProxy 类编写单测

   ```cpp
   using MockHttpServiceProxyPtr = std::shared_ptr<MockHttpServiceProxy>;
   
   class TestFixture : public ::testing::Test {
    public:
     static void SetUpTestCase() { ... }
   
     static void TearDownTestCase() { ... }
   
     void SetUp() override {
       mock_http_proxy_ = ::trpc::GetTrpcClient()->GetProxy<MockGreeterServiceProxy>("mock_http_proxy");
     }
   
     void TearDown() override {}
   
    protected:
     MockHttpServiceProxyPtr mock_http_proxy_;
   };
   
   MockHttpServiceProxyPtr TestFixture::mock_http_proxy_ = nullptr;
   
   TEST_F(TestFixture, GetContentOK) {
     // mock response
     ProtocolPtr rsp = proxy->GetClientCodec()->CreateResponsePtr();
     rsp.SetNonContiguousProtocolBody(...);
     EXPECT_CALL(*proxy, UnaryTransportInvoke(::testing::_, ::testing::_, ::testing::_))
         .Times(1)
         .WillOnce(::testing::SetArgReferee<2>(rsp_data));
   
     std::string url = "xxx";
     rapidjson::Document json;
     auto status = GetContent(mock_http_proxy_, url, &json);
     EXPECT_TRUE(status.OK());
   }
   ```
