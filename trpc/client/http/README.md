[TOC]

# Instructions for using HttpServiceProxy

## 1 HttpServiceProxy
 `HttpServiceProxy` is an extension of `ServiceProxy` for the `http` protocol.

## 2 User interface 
### 2.1 Synchronous calling interface.
#### 2.1.1 GetString
Method: `Get`. Use this interface if the returned content is a string.
#### 2.1.2 Get
Method: `Get`. Use this interface if the returned content is JSON.
#### 2.1.3 Post
Method: `Post`. Use this interface if the returned content is a string/JSON.
#### 2.1.4 PBInvoke
Method: `Post`. Use this interface if both the request and response are PB.
#### 2.1.5 UnaryInvoke
Method: `Post`, http RPC synchronous calling for one-on-one response. Use this interface.

### 2.2 Asynchronous calling interface
#### 2.2.1 AsyncGetString
Method: `Get`. Use this interface if the returned content is a string.
#### 2.2.2 AsyncGet
Method: `Get`. Use this interface if the returned content is JSON.
#### 2.2.3 AsyncPost
Method: `Post`. Use this interface if the returned content is a string/JSON.
#### 2.2.4 AsyncPBInvoke
Method: `Post`. Use this interface if both the request and response are PB.
#### 2.2.5 AsyncUnaryInvoke
Method: `Post`, http RPC asynchronous calling for one-on-one response. Use this interface.

## 3 Setting Custom Headers
Setting headers is at the request level, and it is achieved through the interface of `ClientContext`.
### 3.1 SetHttpHeader
Users can set custom HTTP headers.
### 3.2 GetHttpHeader
Users can get their own set of custom HTTP headers.
### 3.3 Example
A typical `POST` request:
```cpp
trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
ctx->SetTimeout(1000);
ctx->SetHttpHeader("hello", "world");
std::cout << "hello: " << ctx->GetHttpHeader("hello") << std::endl;
proxy->AsyncPost(ctx, "http://example.com/hello", "123");
```

Synchronous calling of `http` RPC, similar to `TrpcServiceProxy`.
```cpp
trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
ctx->SetTimeout(1000);
ctx->SetFuncName("/trpc.test.httpserver.Greeter/SayHello");
ctx->SetHttpHeader("RpcUnaryInvoke_header_k_1", "RpcUnaryInvoke_header_v_1");
trpc::test::httpserver::HelloRequest req;
trpc::test::httpserver::HelloReply rsp;
req.set_msg("Hello, this is http rpc service proxy make UnaryInvoke");
auto status = proxy->UnaryInvoke<trpc::test::httpserver::HelloRequest,trpc::test::httpserver::HelloReply>(ctx, req, &rsp);
```

Asynchronous calling of `http` RPC, similar to `TrpcServiceProxy`.
```cpp
trpc::ClientContextPtr ctx = trpc::MakeClientContext(proxy);
ctx->SetTimeout(1000);
ctx->SetFuncName("/trpc.test.httpserver.Greeter/SayHello");
trpc::test::httpserver::HelloRequest req;
req.set_msg("Hello, this is http rpc service proxy make AsyncUnaryInvoke");
ctx->SetHttpHeader("AsyncRpcUnaryInvoke_header_k_1", "AsyncRpcUnaryInvoke_header_v_1");
auto fu = proxy->AsyncUnaryInvoke<trpc::test::httpserver::HelloRequest,
                                         trpc::test::httpserver::HelloReply>(ctx, req);
// Waits for the future to complete.
fu.Wait();
if (fu.IsFailed()) {
  std::cout << "errcode: " << fu.GetException().GetExceptionCode()
                << ",errmsg: " << fu.GetException().what() << std::endl;
} else {
  auto rsp = fu.GetValue0();
  std::cout << "AsyncUnaryInvoke reply: " << rsp.msg() << std::endl;
}
```
