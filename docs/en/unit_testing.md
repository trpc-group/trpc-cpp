
[中文](../zh/unit_testing.md)

# Overview

To facilitate unit testing of logic code that involves network calls, a commonly used approach is to perform network mocking. Therefore, tRPC-Cpp framework provides testing methods based on [googletest(googlemock)](https://github.com/google/googletest).

Next, we will explain how to write unit tests for both the server and client.

# Server

In terms of usage, it can be divided into synchronous response and asynchronous response scenario.

## Writing unittest in synchronous response scenario

In this scenario, there are no network calls involved, so there iss no need to mock the network. Therefore, writing unit tests is relatively simple. Here is an example:

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

See [GreeterServiceTest_SayHelloOK](/examples/helloworld/greeter_service_test.cc#L32) for the complete example code.

## Writing unittest in asynchronous response scenario

In the scenario of asynchronous response, the user needs to actively call `context->SendUnaryResponse` in the program to send the response. Since this scenario involves network calls, mocking the network is necessary in unit tests.

To facilitate writing unit tests for this scenario, we provides the [MockServerTransport](/trpc/server/testing/server_testing.h#L44) class, which is designed specifically for testing purposes.

To write test with MockServerTransport, first you can should set the transport of service to `MockServerTransport` by calling the `Service::SetServerTransport` interface. Then, you can mock the network calls using the `EXPECT_CALL` syntax from gmock. Here is the pseudocode:

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

See [GreeterServiceTestFixture_SayHelloOK](/examples/unittest/server/async/greeter_service_test.cc#L55) for the complete example code.

Note:

When calling `SendUnaryResponse` to send an asynchronous response, it is necessary to initialize certain members of the `ServerContext` object before using it.

To facilitate the writing of test code, we provides the [ServerContextPtr MakeServerContext(Service* service, ServerCodec* codec)](/trpc/server/testing/server_testing.h#L27) interface to generate a test-specific `ServerContext`.

# Client

Client calls can be categorized into two types: synchronous and asynchronous. In both cases, mocking the network is necessary, and the mocking way are similar. Regarding the specific calling way, there are two main categories:

1. Invoke with RPC interfaces directly, such as tRPC protocol
2. Invoke with ServiceProxy subclasses like HttpServiceProxy, RedisServiceProxy

Next, we will explain how to write unit tests for these two scenarios.

## Writing unittest when invoke with RPC interfaces directly

For this scenario, we provides mock classes that users can utilize to mock RPC interfaces and write unit tests. See [unittest/client](/examples/unittest/client/greeter_client_test.cc) for th complete example code.

### Generate the mock class

As an example using the tRPC protocol, the framework supports generating corresponding mock classes based on the protobuf (pb) files. Users can then use these mock classes to write unittest code.

For example, consider the following pb file, helloworld.proto:

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

By adding `generate_new_mock_code = true` in the trpc_proto_library, you can enable the generation of mock class code.

```bazel
# trpc_proto_library part in BUILD file
trpc_proto_library(
    name = "helloworld_proto",
    srcs = ["helloworld.proto"],
    visibility = [
     "//visibility:public",
    ],
    use_trpc_plugin=True,
    rootpath="@trpc_cpp",
    generate_new_mock_code = True, # This option enables the generation of mock classes.
)
```

After compilation, a header file named `helloworld.trpc.pb.mock.h` will be generated, which includes the implementation of the mock class. The `MockGreeterServiceProxy` class within this file can be used for client-side mock testing.

```cpp
class MockGreeterServiceProxy: public GreeterServiceProxy {
public:
  MOCK_METHOD(::trpc::Status, SayHello, (const ::trpc::ClientContextPtr&, const ::trpc::test::unittest::HelloRequest&, ::trpc::test::unittest::HelloReply*));
  MOCK_METHOD(::trpc::Future<::trpc::test::unittest::HelloReply>, AsyncSayHello, (const ::trpc::ClientContextPtr&, const ::trpc::test::unittest::HelloRequest&));
  MOCK_METHOD(::trpc::Status, SayHello, (const ::trpc::ClientContextPtr&, const ::trpc::test::unittest::HelloRequest&));
};
```

After generating the mock classes, we can use them to write unit tests.

### Writing unittest using mock class

Assuming the code snippet for the client-side invocation logic is as follows:

```cpp
using GreeterServiceProxyPtr = std::shared_ptr<::trpc::test::unittest::GreeterServiceProxy>();

// Synchronous invoke
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

// Asynchronous invoke
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

we use the `MockGreeterServiceProxy` object generated from `helloworld.trpc.pb.mock.h` to mock the RPC interface. The pseudocode is as follows:

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

## Writing unittest when invoke with ServiceProxy subclasses like HttpServiceProxy, RedisServiceProxy

For this scenario, you can perform network mocking by overriding the following virtual interface of the base class ServiceProxy:

```cpp
/// @brief Synchronous call interface oriented to transport, it will be call by UnaryInvoke.
virtual void UnaryTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp);

/// @brief Asynchronous call interface oriented to transport, it will be call by AsyncUnaryInvoke.
virtual Future<ProtocolPtr> AsyncUnaryTransportInvoke(const ClientContextPtr& context,
                                                        const ProtocolPtr& req_protocol);

/// @brief One-way call interface oriented to transport, it will be call by OnewayInvoke.
virtual void OnewayTransportInvoke(const ClientContextPtr& context, const ProtocolPtr& req);
```

Take the following code of synchronous invocation using HttpServiceProxy (the asynchronous invocation method is similar and will not be explained here) as example:

```cpp
using HttpServiceProxyPtr = std::shared_ptr<::trpc::http::HttpServiceProxy>;

Status GetContent(HttpServiceProxyPtr& proxy, const std::string& url, rapidjson::Document* json) {
  auto client_context = ::trpc::MakeClientContext(proxy);
  return proxy->Get(context, url, json);
}
```

The unittest writing steps are as follows:

1. Implement the mock class MockHttpServiceProxy for HttpServiceProxy

   ```cpp
   class MockHttpServiceProxy : public ::trpc::HttpServiceProxy {
    public:
     MOCK_METHOD(void, UnaryTransportInvoke, (const ClientContextPtr& context, const ProtocolPtr& req, ProtocolPtr& rsp),
                 (override));
   
     MOCK_METHOD(Future<ProtocolPtr>, AsyncUnaryTransportInvoke, (const ClientContextPtr& context, const ProtocolPtr& req_protocol), (override));
   };
   ```

2. Write unit tests using the MockHttpServiceProxy class

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
