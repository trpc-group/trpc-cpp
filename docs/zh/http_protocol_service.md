[English](../en/http_protocol_service.md)

# 前言

目前 tRPC-Cpp 支持 HTTP 标准服务和 HTTP RPC 服务（这两个名称仅仅为了区分两种不同服务）。
> HTTP 标准服务是一个普通的 HTTP 服务，不使用 proto 文件定义服务接口，用户需要自己编写代码定义服务接口，注册 URL 路由。
>
> HTTP RPC 服务是 RPC 服务，客户端可以通过 HTTP 协议来访问。服务调用接口由 proto 文件定义，可以由工具生产桩代码。

本文介绍如何基于 tRPC-Cpp （下面简称 tRPC）开发 [HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol)
服务，开发者可以了解到如下内容：

* 如何开发 HTTP 标准服务。
  * 快速上手：快速搭建一个 HTTP 服务。
  * 功能特性一览：RESTful；HTTPS。
  * 基础用法：获取请求、响应常用接口；配置请求路由规则。
  * 进阶用法：分组路由规则；HTTPS；消息体压缩、解压缩；文件上传、下载等。
* 如何开发 HTTP RPC 服务
  * 快速上手：支持 HTTP 客户端访问 tRPC 服务。
* FAQ

# 开发 HTTP 标准服务

## 快速上手

在正式开始前，可以先尝试运行示例代码，直观感受下 HTTP 服务运行过程 。

### 体验 HTTP 服务

示例：[http](../../examples/features/http)

方法：进入 tRPC-Cpp 代码仓库主目录，然后运行下面的命令（感谢耐心等待一小会，此过程会经历代码下载、编译、启动运行阶段，会耗费一点点时间）。

```shell
sh examples/features/http/run.sh
```

示例程序输出：

```text
# 部分过程输出片段内容
response content: hello world!
response content: {"msg":"hello world!"}
response content: {"msg": "hello world!", "age": 18, "height": 180}

# 测例运行成功或失败状态（1： 成功，0：失败）
name: GET string, ok: 1
name: GET json, ok: 1
name: GET http response, ok: 1
name: GET http response(not found), ok: 1
name: HEAD string, ok: 1
name: POST string, ok: 1
name: POST json, ok: 1
name: POST string, then wait http response, ok: 1
name: UNARY invoking, ok: 1
final result of http calling: 1
```

接下来看看 HTTP 服务的开发过程。

### 开发 HTTP 服务的基本步骤

这里用一个简单的 `Hello World` 例子来介绍开发 HTTP 服务的基本步骤。

示例 [http_server.cc](../../examples/features/http/server/http_server.cc)。

基本功能：

* 支持 HTTP 客户端使用 `HEAD or GET or POST` 等方法访问。
* 例如，`POST`  `Hello world!` 将得到 `Hello world!` 响应。

基本步骤：

1. 实现 `HttpHandler` 接口。
2. 设置请求路由。
3. 向 Server 注册一个 `HttpService` 实例。

### 实现过程

1. `FooHandler` 实现 `HttpHandler` 接口 ，处理 HTTP 请求，并设置 HTTP 响应内容。

```cpp
class FooHandler : public ::trpc::http::HttpHandler {
 public:
  ::trpc::Status Get(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                     ::trpc::http::Response* rsp) override {
    rsp->SetContent(greetings_);
    return ::trpc::kSuccStatus;
  }
  ::trpc::Status Head(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                      ::trpc::http::Response* rsp) override {
    rsp->SetHeader("Content-Length", std::to_string(greetings_.size()));
    return ::trpc::kSuccStatus;
  }
  ::trpc::Status Post(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                      ::trpc::http::Response* rsp) override {
    rsp->SetContent(req->GetContent());
    return ::trpc::kSuccStatus;
  }

 private:
  std::string greetings_{"hello world!"};
};
```

`HttpHandler` 是什么？
> `HttpHandler` 提供了一种简单地对 `HTTP Endpoint` 类进行抽象的模式，每个常见 HTTP Method 有对应的接口。
> 各个方法默认的响应为： "501 Not Implemented"。

接口签名大致如下：

```cpp
class trpc::http::HttpHandler {
 public:
  virtual ::trpc::Status Head(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req, ::trpc::http::Response* rsp);
  virtual ::trpc::Status Get(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req, ::trpc::http::Response* rsp); 
  virtual ::trpc::Status Post(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req, ::trpc::http::Response* rsp);
  virtual ::trpc::Status Put(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req, ::trpc::http::Response* rsp); 
  virtual ::trpc::Status Delete(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req, ::trpc::http::Response* rsp); 
  virtual ::trpc::Status Options(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req, ::trpc::http::Response* rsp); 
  virtual ::trpc::Status Patch(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req, ::trpc::http::Response* rsp); 
  // ...
};
```

2. 设置请求路由：将请求 URI 为 `/foo` 的请求路由到 `FooHandler`，支持 `HEAD GET POST` 三种方法。

```cpp
void SetHttpRoutes(::trpc::http::HttpRoutes& r) {
  // ...
  auto foo_handler = std::make_shared<FooHandler>();
  r.Add(::trpc::http::MethodType::GET, ::trpc::http::Path("/foo"), foo_handler);
  r.Add(::trpc::http::MethodType::HEAD, ::trpc::http::Path("/foo"), foo_handler);
  r.Add(::trpc::http::MethodType::POST, ::trpc::http::Path("/foo"), foo_handler);
  // ...
}
```

3. 在应用程序初始化阶段注册一个 `HttpService` 实例，并设置请求路由。

```cpp
class HttpdServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    auto http_service = std::make_shared<::trpc::HttpService>();
    http_service->SetRoutes(SetHttpRoutes);
    RegisterService("default_http_service", http_service);
    return 0;
  }

  void Destroy() override {}
};
```

应用程序启动后，可以通过如下如下 URL 访问。

```bash
# GET /foo HTTP1.1
# e.g: curl http://$ip:$port/foo 
$ hello world!
```

## 功能特性

| 功能/特性                           | 支持状态 |                                备注 |
|:--------------------------------|:----:|----------------------------------:|
| RESTful HEAD/GET/POST ... PATCH | Yes  |                         提供同步/异步接口 |
| HTTPS                           | Yes  |                 支持双向认证，具体看 ssl 配置 |
| 压缩/解压缩                          | Yes  | 提供 gzip, snappy, lz4 等工具，需要用户自行处理 |
| 大文件上传/下载                        | Yes  |                         提供同步/异步接口 |
| HTTP2                           |  No  |                               准备中 |
| HTTP3                           |  No  |                                 - |

## 基础用法

### HTTP Method

可以通过 `GetMethod()` 接口获取 `HTTP Method`。

```cpp
const std::string& GetMethod() const;
```

```cpp
// e.g. 记录请求方法到日志。
TRPC_FMT_INFO("request method: {}", request->GeMethod());
```

如果请求行为 `GET /hello?a=foo&b=bar HTTP/1.1` ，则日志会记录 `request method: GET` 。

### Request URI

可以通过 `GetUrl()` 获取 `Request URI` 。

```cpp
const std::string& GetUrl() const;
```

```cpp
// e.g. 记录 Request URI 到日志。
TRPC_FMT_INFO("request uri: {}", request->GetUrl());
```

如果请求行为 `GET /hello?a=foo&b=bar HTTP/1.1` ，则日志会记录 `request uri: /hello?a=foo&b=bar` 。

### Query String

`Query String` 多个 key-value 键值对，key 区分大小。通过 `Request URI` 来传输，常见形式是 `key1=value1&key2=value2&…`
，易于阅读和修改，常用于传递应用层参数。
通过 `GetQueryParameters()` 获取参数对象。

```cpp
const QueryParameters& GetQueryParameters() const;
```

```cpp
// e.g. 获取 Query String 中的 a 和 b 参数。
auto a = GetQueryParameters().Get("a");
auto b = GetQueryParameters().Get("b");
```

如果请求行为 `GET /hello?a=foo&b=bar HTTP/1.1` ，则 `a= "foo"; b = "bar"` 。

### HTTP Headers

Headers 是多个 key-value 键值对，在 HTTP 协议中， key 不区分大小写。
`Request/Response` 提供增、删、改、查接口。

```cpp
// e.g. 从请求头中获取 User-Agent 值。
auto user_agent = request->GetHeader("User-Agent");
// e.g. 从请求中获取 Content-Type 值。
auto content_type = request->GetHeader("Content-Type");

// e.g. 设置响应的 Content-Type 为 JSON。
response->SetHeader("Content-Type", "application/json");
```

### HTTP Body

请求、响应的消息体是个缓冲区，提供两种接口访问：

* 连续 Buffer: std::string。
* 非连续 Buffer: NonContiguousBuffer。

```cpp
const std::string& GetContent() const;
void SetContent(std::string content);

// Better performance.
const NoncontiguousBuffer& GetNonContiguousBufferContent() const;
void SetNonContiguousBufferContent(NoncontiguousBuffer&& content); 
```

```cpp
// e.g. 从请求中获取 Body
auto& body = request->GetContent();

// e.g. 设置响应 Body 为 "Hello world!"。
response->SetContent("Hello world!");
```

### Response Status

status 是 Response 特有的字段，标记 HTTP Response
完成状态，值定义在 [http/status.h](../../trpc/util/http/status.h)。

```cpp
int GetStatus() const; 
void SetStatus(int status); 
```

```cpp
// e.g. 设置响应码为 302 重定向。
response->SetStatus(::trpc::http::ResponseStatus::kFound);
response->SetHeader("Location", "https://github.com/");
```

### 基本的路由匹配规则

当前支持如下匹配规则：

* 完全匹配
* 前缀匹配
* 正则匹配
* 占位符匹配

#### 完全匹配

直接在 `Path(path_value)` 中填写路径时，默认是采取完全匹配。即当请求 URI 中的 Path 与路由设置的 `path_value`
值完全一致时，请求才会被相应的 handler 处理。

```cpp
 r.Add(trpc::http::MethodType::GET, trpc::http::Path("/hello/img_x"), handler);
```

```bash
curl http://$ip:$port/hello/img_x -X GET  # 响应码 200
curl http://$ip:$port/hello/img -X GET    # 响应码 404
```

#### 前缀匹配

通过 `Path(path_value).Remainder("path")` 追加 path 占位符，可使用前缀匹配， 即仅当请求 URI 中的 Path 前缀 与
Path 一致，请求也可被相应的 handler 处理。
> `Remainder` 中内容仅为占位符，无实际含义，建议默认填写为 `path`。

```cpp
 r.Add(trpc::http::MethodType::GET, trpc::http::Path("/hello").Remainder("path"), handler_hello);
```

```bash
curl http://$ip:$port/hello/img -X GET    # 响应码 200
curl http://$ip:$port/hello/img/1 -X GET  # 响应码 200
curl http://$ip:$port/hello -X GET        # 响应码 200

# 如果前缀不匹配，则访问失败。
curl http://$ip:$port/hello1 -X GET       # 响应码 404
curl http://$ip:$port/hello1/img -X GET   # 响应码 404
```

#### 正则匹配

通过使用 `<regex()>` 来包裹 `path_value` 时， 可使用正则匹配，即实际填写的路径值为一个正则表达式，当请求 URI 中的 Path
符合该正则表达式时，请求会被相应的 handler 处理。

```cpp
r.Add(trpc::http::MethodType::GET, trpc::http::Path("<regex(/img/[a-z]_.*)>"), handler);
```

```bash
curl http://$ip:$port/img/d_123/ccd -X GET`    # 响应码 200
curl http://$ip:$port/img/123_123/ccd -X GET`  # 响应码 404
```

#### 占位符匹配

通过使用 `<ph()>` 包裹 `path_value`  时，可使用占位符匹配。即实际填写的路径值允许包含占位符， 当请求 URI 中的 Path
符合该正则表达式时，请求会被相应的 handler 处理，这对于路径参数非常适用。

下面 URI 路径的占位符包括 `channel_id` 和 `client_id`：

```cpp
r.Add(trpc::http::MethodType::GET, trpc::http::Path("<ph(/channels/<channel_id>/clients/<client_id>)>"), handler);
```

可以方便取出占位符的值：

```cpp
::trpc::Status Get(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                   ::trpc::http::Response* rsp) override {
  auto channel_id = req->GetParameters().Path("channel_id");
  auto client_id = req->GetParameters().Path("client_id");
  // ...
  return ::trpc::kSuccStatus;
}
```

```bash
curl http://$ip:$port/channels/xyz/clients/123 -X GET  # 响应码 200
curl http://$ip:$port/channels/clients -X GET          # 响应码 404
```

**注意:**
  > 占位符的符号仅支持字母、数字、下划线。

## 进阶用法

本节介绍进阶的用法，比如 分组路由规则、HTTPS、消息体压缩和解压缩、大文件上传和下载等。

### 分组路由规则

使用 `HttpHandlerGroups` 工具类，可以提供可读性更高的简易 HTTP 路由注册方法，适用于需要提供多层次路径的服务，例如，RESTful
Services。

HttpHandlerGroups 目前仅支持 `完全匹配` 和 `占位符匹配` 两种形式，使用时需额外引入 `trpc/util/http/http_handler_groups.h`
头文件。

新增宏命令如下，具体使用方法详见下方代码示例：

| 宏名称                          | 功能                               | 说明                                                                | 参数                                         | 示例                                                                                      |
|------------------------------|----------------------------------|-------------------------------------------------------------------|--------------------------------------------|-----------------------------------------------------------------------------------------|
| TRPC_HTTP_ROUTE_HANDLER      | 声明一层 HTTP RESTful 路由，可嵌套使用       | 默认引用捕获所有外界参数（如*全局变量、成员变量*），**无法捕获超过最外层函数的scope**（如*局部变量*）         | { 具体的路由声明代码 }                              | ` TRPC_HTTP_ROUTE_HANDLER({ code; }) `                                                  |
| TRPC_HTTP_ROUTE_HANDLER_ARGS | 声明一层带参数捕获的 HTTP RESTful 路由，可嵌套使用 | 默认引用捕获所有外界参数（如*全局变量、成员变量*）和**额外传入的参数**（**!!#ff0000 可以!!**是*局部变量*） | ( 所有传入参数的值 ), ( 各个参数的类型和名称 ) { 具体的路由声明代码 } | `TRPC_HTTP_ROUTE_HANDLER_ARGS((a, 1, a+1), (double a, int b, auto c) { code; })`        |
| TRPC_HTTP_HANDLER            | 绑定指定对象和其成员方法为一个路由终点              | -                                                                 | 对象的shared_ptr, 成员方法名（以`类名::方法名`填写）         | ` TRPC_HTTP_HANDLER(coffee_pot_controller, CoffeePotController::Brew) `                 |
| TRPC_HTTP_STREAM_HANDLER     | 绑定指定对象和其成员方法为一个*流式*路由终点          | -                                                                 | 对象的shared_ptr, 成员方法名（以`类名::方法名`填写）         | ` TRPC_HTTP_STREAM_HANDLER(stream_download_controller, StreamDownloadController::Get) ` |

先来看下完整示例：

```cpp
void SetHttpRoutes(trpc::http::HttpRoutes& routes) {
  // 建议加入 clang-format 控制字段, 避免不理想的代码格式化结果
  // clang-format off
  trpc::http::HttpHandlerGroups(routes).Path("/", TRPC_HTTP_ROUTE_HANDLER({  // TRPC_HTTP_ROUTE_HANDLER 宏声明一层路由, 可多层嵌套
    // 默认 TRPC_HTTP_ROUTE_HANDLER 会捕获所有外界引用, 但受函数内部类限制, 无法捕获超过最外层函数的 scope
    Path("/api", TRPC_HTTP_ROUTE_HANDLER({  // 每层 Path 可以是普通字符串, 或是以尖括号声明占位符, 见下方, 前缀和后缀 "/" 可忽略
      auto hello_world_handler = std::make_shared<ApiHelloWorldHandler>();
      Path("/user", TRPC_HTTP_ROUTE_HANDLER({
        Get<ApiUserHandler>();              // Get  - /api/user - 此处将会自动注册和管理一个 ApiUserHandler 对象（必须为可默认构造的）
        Post<ApiUserHandler>("/");          // Post - /api/user - 同个 HttpHandlerGroups 中对同类型复用相同对象
        Delete<ApiUserHandler>("/<user_id>");  // Delete - /api/user/<user_id> - 复用对象 - 路径占位符名为 user_id
      });
      Path("/hello", TRPC_HTTP_ROUTE_HANDLER_ARGS((hello_world_handler), (auto h) {
        // 使用 TRPC_HTTP_ROUTE_HANDLER_ARGS((captures...), (args...) { ... }) 可自定义需捕获的值和捕获后的变量名
        Head("/hello_world", h);  // Head - /api/hello_world - 也可以通过 std::shared_ptr 注册对象
      });
      Get("/teapot", [](auto ctx, auto req, auto rep) {                                    // 第一个匹配的路由优先, /api/teapot 会到这里
        rep->SetStatus(::trpc::http::ResponseStatus::kIAmATeapot);
      });
      Get("<path>", TRPC_HTTP_HANDLER(coffee_pot_controller, CoffeePotController::Brew));  // 而 /api/coffee_pot 会到这里
    });
  });
  // clang-format on
}
```

#### 注册 `HttpHandler`

使用 `TRPC_HTTP_ROUTE_HANDLER` 宏注册 `HttpHandler` 接口路由。

```cpp
class ApiUserHandler : public ::trpc::http::HttpHandler {
 public:
  void Get(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req, ::trpc::http::Response* rsp) override {
    // return user info.
  }

  void Post(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::HttpRequestPtr& req, ::trpc::http::Response* rsp) override {
    // create new user.
  }
};

// ...

::trpc::http::HttpHandlerGroups(routes).Path("/", TRPC_HTTP_ROUTE_HANDLER({
  Path("/api", TRPC_HTTP_ROUTE_HANDLER({
    Path("/user", TRPC_HTTP_ROUTE_HANDLER({
      Get<ApiUserHandler>();      // Get  - /api/user - 此处将会自动注册和管理一个 ApiUserHandler 对象
      Post<ApiUserHandler>("/");  // Post - /api/user - 同个 HttpHandlerGroups 中对同类型复用相同对象
    });
    auto hello_world_handler = std::make_shared<ApiHelloWorldHandler>();
    Head("/hello_world", hello_world_handler);  // Head - /api/hello_world - 也可以通过 std::shared_ptr 注册对象
  });
});
// ...
```

#### 注册 `HttpController`

使用 `TPRC_HTTP_HANDLER` 宏，用户也可以方便地注册类似于 `Controller` 类接口到路由中:

提示： Controller 未继承 `HttpHandler` 类，只是接口签名相同。

```cpp
class UserController {
 public:
  void Get(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req, ::trpc::http::Response* rsp) {
    // return user info.
  }

  void Post(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::HttpRequestPtr& req, ::trpc::http::Response* rsp) {
    // create new user.
  }
};

// ...
trpc::http::HttpHandlerGroups(routes).Path("/", TRPC_HTTP_ROUTE_HANDLER({
  Path("/api", TRPC_HTTP_ROUTE_HANDLER({
    Path("/user", TRPC_HTTP_ROUTE_HANDLER({
      auto user_controller = std::make_shared<UserController>();
      Get(TRPC_HTTP_HANDLER(user_controller, UserController::GetUser));      // Get  - /api/user
      Post(TRPC_HTTP_HANDLER(user_controller, UserController::CreateUser));  // Post - /api/user
    });
  });
});
// ...
```

#### 注册 `Lambda` 函数

如果需要，用户可以直接注册 lambda 函数到路由之中。

```cpp
trpc::http::HttpHandlerGroups(routes).Path("/api", TRPC_HTTP_ROUTE_HANDLER({
   Get("/teapot", [](auto ctx, auto req, auto rep) { 
     rep->SetStatus(::trpc::http::ResponseStatus::kIAmATeapot);
   });
   Get("<path>", handler);
});
```

### 压缩/解压缩消息体

为了降低网页传输时间，加快页面渲染速度，HTTP 服务可能会压缩响应消息体。
tRPC 当前不会自动压缩或者解压缩消息体，主要基于如下考虑：

* 通用性，这个操作交由用户自行处理会更灵活。
* 压缩、解压缩代码不是很复杂，tRPC 提供了压缩和解压缩工具，当前支持 gzip，lz4， snappy，zlib
  等 [compressor](../../trpc/compressor)

### 处理 HTTPS 请求

HTTPS 是 HTTP over SSL 的简称，可以通过如下方式开启 SSL。

* 编译代码时开启 SSL 编译选项。

  > 使用 bazel build 时，增加 --define trpc_include_ssl=true 编译参数。
  > 提示：也可以加到 .bazelrc 文件中。

  **提示：tRPC 基于 OpenSSL 支持 HTTPS，请确保编译、运行环境正确安装 OpenSSL 。**

  ```cpp
  // e.g. 
  bazel build --define trpc_include_ssl=true //https:http_server
  ```

* 在配置文件中设置 SSL 相关配置，具体配置项如下：

  | 名称               | 功能       | 取值范围                                    | 默认值               | 可选状态       | 说明                          |
  |------------------|----------|-----------------------------------------|-------------------|------------|-----------------------------|
  | cert_path        | 证书路径     | 不限，xx/path/to/server.pem                | null              | `required` | 在启用 SSL 时，如果不设置正确路径，服务会启动失败 |
  | private_key_path | 私钥路径     | 不限，xx/path/to/server.key                | null              | `required` | 在启用 SSL 时，如果不设置正确路径，服务会启动失败 |
  | ciphers          | 加密套件     | 不限                                      | null              | `required` | 在启用 SSL 时，如果不正确设置，服务会启动失败   |
  | enable           | 是否启用SSL  | {true, false}                           | false             | optional   | 建议在配置项明确指定，明确意图             |
  | mutual_auth      | 是否启用双向认证 | {true, false}                           | false             | optional   | -                           |
  | ca_cert_path     | CA证书路径   | 不限，xx/path/to/ca.pem                    | null              | optional   | 双向认证是开启有效                   |
  | protocols        | SSL协议版本  | {SSLv2, SSLv3, TLSv1, TLSv1.1, TLSv1.2} | TLSv1.1 + TLSv1.2 | optional   | -                           |
  
  举个例子：
  
  ```yaml
  # @file: trpc_cpp.yaml
  # ...
  server:
    service:
      - name: default_http_service
        network: tcp
        ip: 0.0.0.0
        port: 24756
        protocol: http
        # ...
        ## <-- 新增的 SSL 配置项
        ssl:
          enable: true  # 可选配置（默认为 false，表示禁用 SSL）
          cert_path: ./https/cert/server_cert.pem # 必选配置
          private_key_path: ./https/cert/server_key.pem # 必选配置
          ciphers: HIGH:!aNULL:!kRSA:!SRP:!PSK:!CAMELLIA:!RC4:!MD5:!DSS # 必选配置
          # mutual_auth: true # 可选配置（默认为 false，表示不开启双向认证）
          # ca_cert_path: ./https/cert/xxops-com-chain.pem # 可选配置，双向认证的 CA 路径
          # protocols: # 可选配置
          #   - SSLv2
          #   - SSLv3
          #   - TLSv1
          #   - TLSv1.1
          #  - TLSv1.2
          ## --> 新增的SSL配置项
  # ...
  ```

### 服务异步响应客户端

如果服务端处理 HTTP Request 的逻辑异步执行，然后用户期望自己主动回复响应给客户端，而不是让 tRPC 自动回复 HTTP 响应，可以采用如下方式：

* 关闭 tRPC 回包: context->SetResponse(false) 。
* 回包时，将 HTTP Response 序列化成非连续 Buffer，然后调用 context->SendResponse(buffer) 发送响应包。

  ```cpp
  // ServerContext 接口
  void SetResponse(bool is_response);
  Status SendResponse(NoncontiguousBuffer&& buffer);
  
  // HTTP Response 接口
  bool SerializeToString(NoncontiguousBuffer& buff) const;
  ```

* 代码片段：
  
  ```cpp
  // 使用示例
  trpc::Status Get(const trpc::ServerContextPtr& context, const trpc::http::RequestPtr req, trpc::http::Response* rep) {
    // ...
    // 关闭 tRPC 主动回包
    context->SetResponse(false);
    // 异步操作，比如 Future 模式异步调用
    DoAsyncWork(context,req);
    // ...
    return trpc::kSucctStatus;
  }
  
  void DoAsyncWork(const trpc::ServerContextPtr& context,const trpc::http::RequestPtr& req){
    // ...
    
    trpc::http::Response http_response;
    // 生成正常响应的函数签名  void GenerateCommonReply(HttpRequest* req);
    http_response.GenerateCommonReply(req.get());
  
    // 设置响应内容，比如
    http_response.SetContent("DoAsyncWork");
      
    // 将 HTTP Response 序列化为非连续 Buffer
    trpc::NoncontiguousBuffer out;
    http_response.SerializeToString(out);
      
    // 使用 ServerContext 主动回包
    context->SendResponse(std::move(out));
  }
  ```

### 大文件上传 + 下载

在 HTTP 服务中，有部分场景需要读取或者发送大文件的场景，将文件完整读入内存压力较大且效率较低，对于大文件的上传可行性不高。
tRPC 提供一套 HTTP 流式读取/写入数据分片的接口，可以分片接收/发送大文件。

* 对于已经知晓长度的大文件，设置 `Content-Length: $length` 后分块发送（当然也可以使用 chunked 分块发送，如果对端支持的话）。
* 对于未知长度的大文件，设置 `Transfer-Encoding: chunked` 后分块发送。

具体可以参考 [http 上传 下载](./http_protocol_upload_download_service.md)。

# 开发 HTTP RPC 服务

## 在 HTTP 协议下提供 tRPC 服务

某些场景下，一些 RPC 服务可能需要对外支持 HTTP 客户端。即，通过 HTTP 协议传输 tRPC 协议头部和载荷。
可以通过修改配置项，无需修改代码，轻松实现：`protocol: trpc` -> `protocol: trpc_over_http` 。

如果需要同时支持 tRPC 和 HTTP 客户端，可以在服务初始化阶段注册两个 RPC Service 实例。

* 代码片段：

  ```cpp
  class HelloWorldServer : public ::trpc::TrpcApp {
   public:
    int Initialize() override {
      // ...
      std::string service_name = "trpc.test.helloworld.Greeter"; 
      RegisterService(service_name, std::make_shared<GreeterServiceImpl>());
  
      std::string service_over_http_name = "trpc.test.helloworld.GreeterHTTP"; 
      RegisterService(service_over_http_name, std::make_shared<GreeterServiceImpl>());
      // ...
      return 0;
    }
  };
  ```

* 配置片段：

  ```yaml
  # @file: trpc_cpp.yaml
  # ...
  service:
    - name: trpc.test.helloworld.Greeter
      protocol: trpc
      network: tcp
      ip: 0.0.0.0
      port: 12345
    - name: trpc.test.helloworld.GreeterHTTP
      protocol: trpc_over_http
      network: tcp
      ip: 0.0.0.0
      port: 23456
  # ...
  ```

## 透传信息处理

对于HTTP RPC 服务，框架在编解码时会对透传信息进行如下处理：
* 解码：框架在收到请求包时，会从HTTP头部中取出名称为`“trpc-trans-info”`的数据，将其内容按JSON格式进行解析，然后将解析到的键值对作为请求的透传信息。用户可以通过`“ServerContext::GetPbReqTransInfo”`接口获取。
* 编码：框架在进行回包响应时，会将用户通过`“ServerContext::AddRspTransInfo”`接口设置的透传信息，转换成一个名称为
`“trpc-trans-info”`，值为JSON串的HTTP头部。

注意框架默认不会对透传信息中的值进行base64编解码，如果有需要，可以通过添加`“trpc_enable_http_transinfo_base64”`编译选项来开启：
```
build --define trpc_enable_http_transinfo_base64=true
```

# FAQ

## 如果匹配所有路径需要怎么写路由规则？

```cpp
// e.g. :
r.Add(trpc::http::MethodType::POST, trpc::http::Path("").Remainder("path"), handler);
```

## 为什么开启 SSL 后启动服务出现 coredump ?

请确保 ssl 配置项中的证书、私钥等文件路径配置正确。

## 使用 curl 上传 1KB+ 数据时，为什么会多等待 1s ?

curl/libcurl 在上传数据时，当 body 大于 1K 时，会先询问下服务器是否允许发送大数据包（请求头中增加 "Expect： 100 continue"）。
如果允许则回复一个 100 continue 码，然后libcurl 继续发送请求体。
如果服务端不响应 100 continue，等待1s后，会继续将请求体发给服务端。
当前 tRPC HTTP Server 侧没有响应 Expect: 100 continue，所以 curl 只能等待1s后发送数据。

规避办法:

1. curl 测试时，发送一个带空值的 "Expect:" ，curl -H "Expect:" -X POST -T "ddd" $url
2. 使用 tRPC HTTP 上传、下载相关接口实现相关服务接口，这类接口支持响应 100 Continue。
