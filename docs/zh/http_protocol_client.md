[English](../en/http_protocol_client.md)

# 前言

tRPC-Cpp 默认支持 tRPC 协议，同时也支持 HTTP 协议。
用户可在 HTTP 请求中发送 Plain Text 或者 JSON 等数据来访问标准 HTTP 服务，也可在请求中发送 Protobuf 数据来访问
RPC 服务。
这可以使一个 RPC 服务同时支持 tRPC 和 HTTP 协议，当前支持通过 HTTP 客户端访问 tRPC 服务。

本文介绍如何基于 tRPC-Cpp （下面简称 tRPC）访问 HTTP 服务，开发可以了解到如下内容：

* 访问 HTTP 标准服务
  * 快速上手：使用一个 HTTP Client 访问 HTTP 服务。
  * 基础用法：设置请求常用接口；获取响应常用接口。
  * 进阶用法：发送 HTTPS请求；请求消息压缩、响应消息解压缩；大文件上传、下载。
* 访问 HTTP RPC 服务
* FAQ

# 访问 HTTP 标准服务

## 快速上手

在正式开始前，可以先尝试运行示例代码，直观感受下 HTTP 服务运行过程。
另外，成功运行的服务可以用作客户端访问的目标服务对象，方便验证客户端访问接口的结果。

### 体验 HTTP 服务

示例：[http](../../examples/features/http)

方法：进入 tRPC 代码仓库主目录，然后运行下面的命令。

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

简单看下客户端的运行输出结果：发出 `GET, HEAD, POST` 请求并接收到了对应的响应。另外，还有一个 Unary RPC 成功执行了。

接下来看看访问 HTTP 服务的步骤。

### 访问 HTTP 服务的基本步骤

1. 获取 `HttpServiceProxyPtr` 对象 `proxy`：使用 `GetTrpcClient()->GetProxy<HttpServiceProxy>(...)`。
2. 创建 `ClientContextPtr` 对象 `context`：使用 `MakeClientContext(proxy)`。
3. 调用期望访问的接口并检查调用返回码，比如 `GetString or PostString` 等（可根据业务场景选择使用同步接口或者异步接口）。

下面以 `fiber 协程` 为例，介绍 `GetString` 访问过程。

示例：[http_client.cc](../../examples/features/http/client/client.cc)

### 访问过程

1. 获取 `HttpServiceProxyPtr` 对象 `proxy`：使用 `::trpc::GetTrpcClient()->GetProxy<HttpServiceProxy>(...)`。

   ```cpp
   std::string service_name{"http_client_xx"};
   ::trpc::ServiceProxyOption option;
   
   // ... 此处省略  option 初始化过程
   // 此处的 proxy 获取后在其他地方可继续使用，不需要每次都获取
   auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(servie_name, option);
   ```

   > 创建 HttpServiceProxyPtr 有 3 种方式：
   >
   > -1- 代码中设置 `ServiceProxyOption` 对象。
   >
   > -2- 使用 yaml 配置文件指定 service 配置项。（推荐）
   >
   > -3- 通过回调函数初始化 `ServiceProxyOption`  + yaml 配置项。
   >
   > 可参考 [client_guide](./client_guide.md)

2. 创建 `ClientContextPtr` 对象 `context`：使用 `MakeClientContext(proxy)`。

   ```cpp
   auto ctx = ::trpc::MakeClientContext(proxy);
   ```

3. 调用 `GetString` 。

   ```cpp
   // ...
   auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(servie_name, option);
   auto ctx = ::trpc::MakeClientContext(proxy);
   std::string rsp_str;
   auto status = proxy->GetString(ctx, "http://example.com/foo", &rsp_str);
   if (!status.OK()) {
     // Error ...
   } else {
     // Ok ...
   }
   // ...
   ```

## 基础用法

`HttpServiceProxy` 提供了 `Head Get Post Put Delete Options`
等接口，为了使用方便，还封装了一些获取特定对象的接口，比如常用的 `String` 和 `JSON` 对象。

这些接口一般适用于简单的场景，比如 GET 或者 POST 一个 String 或者 JSON。

针对不同编程模型，提供了同步和异步接口。 `fiber` 协程模式下推荐使用同步接口，在 `merge or separate` 线程模型下推荐使用异步接口。

下面先快速浏览下接口列表

### HttpServiceProxy 接口列表

关于接口命名问题：部分接口命名可能不优雅，会提高理解成本，文档会尽量注明清楚。

#### GET

Get 接口从服务端获取一个 JSON/字符串，提供同步调用和异步调用接口。

提示：下面的 Get2 仅仅是一个接口名，并非是使用 HTTP2 协议。

| 类/对象             | 接口名称                                                                                                     | 功能                      | 参数                                             | 返回值                            | 备注   |
|------------------|----------------------------------------------------------------------------------------------------------|-------------------------|------------------------------------------------|--------------------------------|------|
| HttpServiceProxy | Status Get(const ClientContextPtr&amp; context, const std::string&amp; url, rapidjson::Document* json)   | 使用 GET 方法获取一个 JSON 响应消息 | context 客户端上下文<br> url 资源位置 <br> json 存放响应消息   | Status                         | 同步接口 |
| HttpServiceProxy | Status GetString(const ClientContextPtr&amp; context, const std::string&amp; url, std::string* rspStr)   | 使用 GET 方法获取一个字符串响应消息    | context 客户端上下文<br> url 资源位置 <br> rspStr 存放响应消息 | Status                         | 同步接口 |
| HttpServiceProxy | Status Get2(const ClientContextPtr&amp; context, const std::string&amp; url, HttpResponse* rsp)          | 使用 GET 方法获取 HTTP 响应     | context 客户端上下文<br> url 资源位置 <br> rsp 存放HTTP响应  | Status                         | 同步接口 |
| HttpServiceProxy | Future&lt;rapidjson::Document> AsyncGet(const ClientContextPtr&amp; context, const std::string&amp; url) | 使用 GET 方法获取一个 JSON 响应消息 | context 客户端上下文<br> url 资源位置                    | Future&lt;rapidjson::Document> | 异步接口 |
| HttpServiceProxy | Future&lt;std::string> AsyncGetString(const ClientContextPtr&amp; context, const std::string&amp; url)   | 使用 GET 方法获取一个字符串响应消息    | context 客户端上下文<br> url 资源位置 <br>               | Future&lt;std::string>         | 异步接口 |
| HttpServiceProxy | Future&lt;HttpResponse> AsyncGet2(const ClientContextPtr&amp; context, const std::string&amp; url)          | 使用 GET 方法获取 HTTP 响应     | context 客户端上下文<br> url 资源位置 <br>               | Future&lt;HttpResponse>        | 异步接口 |

#### POST

Post 接口支持发送一个JSON/字符串请求给服务端，然后从服务端获取一个JSON/字符串响应。

提示：下面的 Post2 仅仅是一个接口名，并非是使用 HTTP2 协议。

| 类/对象             | 接口名称                                                                                                                                           | 功能                                       | 参数                                                          | 返回值                            | 备注           |
|------------------|------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------|-------------------------------------------------------------|--------------------------------|--------------|
| HttpServiceProxy | Status Post(const ClientContextPtr&amp; context, const std::string&amp; url, const rapidjson::Document&amp; data, rapidjson::Document* rsp)    | 使用 POST 方法发送一个 JSON 请求消息，并获取一个 JSON 响应消息 | context 客户端上下文<br> url 资源位置 <br> data 请求消息 <br> rsp 存放响应消息  | Status                         | 同步接口         |
| HttpServiceProxy | Status Post(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp; data, std::string* rsp)                    | 使用 POST 方法发送一个字符串请求消息，并获取一个字符串响应消息       | context 客户端上下文<br> url 资源位置 <br> data 请求消息 <br> rsp 存放响应消息  | Status                         | 同步接口         |
| HttpServiceProxy | Status Post(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp;&amp; data, std::string* rsp)               | 使用 POST 方法发送一个字符串请求消息，并获取一个字符串响应消息       | context 客户端上下文<br> url 资源位置 <br> data 请求消息 <br> rsp 存放响应消息  | Status                         | 同步接口，性能优化接口  |
| HttpServiceProxy | Status Post2(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp; data, HttpResponse* rsp)                  | 使用 POST 方法发送一个字符串请求消息，并获取一个 HTTP 响应      | context 客户端上下文<br> url 资源位置 <br> data 请求消息 <br> rsp 存放响应消息  | Status                         | 同步接口         |
| HttpServiceProxy | Status Post2(const ClientContextPtr&amp; context, const std::string&amp; url,  std::string&amp;&amp; data, HttpResponse* rsp)                  | 使用 POST 方法发送一个字符串请求消息，并获取一个 HTTP 响应      | context 客户端上下文<br> url 资源位置 <br> data 请求消息 <br> rsp 存放响应消息  | Status                         | 同步接口，性能优化接口  |
| HttpServiceProxy | Status Post(const ClientContextPtr&amp; context, const std::string&amp; url, NoncontiguousBuffer&amp;&amp; data, NoncontiguousBuffer* body)    | 使用 POST 方法发送一个字符串请求消息，并获取一个字符串响应消息       | context 客户端上下文<br> url 资源位置 <br> data 请求消息 <br> body 存放响应消息 | Status                         | 同步接口，性能优化接口  |
| HttpServiceProxy | Future&lt;rapidjson::Document> AsyncPost(const ClientContextPtr&amp; context, const std::string&amp; url, const rapidjson::Document&amp; data) | 使用 POST 方法发送一个 JSON 请求消息，并获取一个 JSON 响应消息 | context 客户端上下文<br> url 资源位置 <br> data 请求消息                  | Future&lt;rapidjson::Document> | 异步接口         |
| HttpServiceProxy | Future&lt;std::string> AsyncPost(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp; data)                 | 使用 POST 方法发送一个字符串请求消息，并获取一个字符串响应消息       | context 客户端上下文<br> url 资源位置 <br> data 请求消息                  | Future&lt;std::string>         | 异步接口         |
| HttpServiceProxy | Future&lt;std::string> AsyncPost(const ClientContextPtr&amp; context, const std::string&amp; url, std::string&amp;&amp; data)                  | 使用 POST 方法发送一个字符串请求消息，并获取一个字符串响应消息       | context 客户端上下文<br> url 资源位置 <br> data 请求消息                  | Future&lt;std::string>         | 异步接口，性能优化接口  |
| HttpServiceProxy | Future&lt;NoncontiguousBuffer> AsyncPost(const ClientContextPtr&amp; context, const std::string&amp; url, NoncontiguousBuffer&amp;&amp; data)  | 使用 POST 方法发送一个字符串请求消息，并获取一个字符串响应消息       | context 客户端上下文<br> url 资源位置 <br> data 请求消息                  | Future&lt;NoncontiguousBuffer> | 异步接口, 性能优化接口 |
| HttpServiceProxy | Future&lt;HttpResponse> AsyncPost2(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp; data)               | 使用 POST 方法发送一个字符串请求消息，并获取一个字符串响应消息       | context 客户端上下文<br> url 资源位置 <br> data 请求消息                  | Future&lt;HttpResponse>        | 异步接口         |
| HttpServiceProxy | Future&lt;HttpResponse> AsyncPost2(const ClientContextPtr&amp; context, const std::string&amp; url, std::string&amp;&amp; data)                | 使用 POST 方法发送一个字符串请求消息，并获取一个字符串响应消息       | context 客户端上下文<br> url 资源位置 <br> data 请求消息                  | Future&lt;HttpResponse>        | 异步接口，性能优化接口  |

#### HEAD  PUT  OPTIONS  PATCH  DELETE

*提示：下面的 Put2 仅仅是一个接口名，并非是使用 HTTP2 协议。*

```cpp
/// @brief Gets an HTTP response of an HTTP HEAD method request from HTTP server.
Status Head(const ClientContextPtr& context, const std::string& url, HttpResponse* rsp);

/// @brief Gets an HTTP response of an HTTP OPTIONS method request from HTTP server.
Status Options(const ClientContextPtr& context, const std::string& url, HttpResponse* rsp);

/// @brief Puts a JSON as the request body to HTTP server, then gets the JSON content of response.
Status Put(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data, rapidjson::Document* rsp);
/// @brief Puts a string as the request body to HTTP server, then gets the string content of response.
Status Put(const ClientContextPtr& context, const std::string& url, const std::string& data, std::string* rsp_str);
Status Put(const ClientContextPtr& context, const std::string& url, std::string&& data, std::string* rsp_str);
Status Put(const ClientContextPtr& context, const std::string& url, NoncontiguousBuffer&& data, NoncontiguousBuffer* body);
/// @brief Puts a string as the request body to HTTP server, then gets the HTTP response.
Status Put2(const ClientContextPtr& context, const std::string& url, const std::string& data, HttpResponse* rsp);
Status Put2(const ClientContextPtr& context, const std::string& url, std::string&& data, HttpResponse* rsp);

/// @brief Sends an HTTP PATCH request with a string as the request body to HTTP server, then gets the HTTP response.
Status Patch(const ClientContextPtr& context, const std::string& url, const std::string& data, HttpResponse* rsp);
Status Patch(const ClientContextPtr& context, const std::string& url, std::string&& data, HttpResponse* rsp);

/// @brief Sends an HTTP DELETE request with a string as the request body to HTTP server, then gets the HTTP response.
Status Delete(const ClientContextPtr& context, const std::string& url, const std::string& data, HttpResponse* rsp);
Status Delete(const ClientContextPtr& context, const std::string& url, std::string&& data, HttpResponse* rsp);

// 与之对应的异步接口：
Future<HttpResponse> AsyncHead(const ClientContextPtr& context, const std::string& url);

Future<HttpResponse> AsyncOptions(const ClientContextPtr& context, const std::string& url);

Future<rapidjson::Document> AsyncPut(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data);
Future<std::string> AsyncPut(const ClientContextPtr& context, const std::string& url, const std::string& data);
Future<std::string> AsyncPut(const ClientContextPtr& context, const std::string& url, std::string&& data);
Future<NoncontiguousBuffer> AsyncPut(const ClientContextPtr& context, const std::string& url, NoncontiguousBuffer&& data);
Future<HttpResponse> AsyncPut2(const ClientContextPtr& context, const std::string& url, const std::string& data);
Future<HttpResponse> AsyncPut2(const ClientContextPtr& context, const std::string& url, std::string&& data);

Future<HttpResponse> AsyncPatch(const ClientContextPtr& context, const std::string& url, const std::string& data);
Future<HttpResponse> AsyncPatch(const ClientContextPtr& context, const std::string& url, std::string&& data);
Future<HttpResponse> AsyncDelete(const ClientContextPtr& context, const std::string& url, const std::string& data);
Future<HttpResponse> AsyncDelete(const ClientContextPtr& context, const std::string& url, std::string&& data);
```

### 暂未支持 CONNECT 方法

tRPC-Cpp 侧暂未实现 HTTP CONNECT 相关逻辑。

### 接口返回值 Status

`HttpServiceProxy` 提供的同步接口返回 `Status` 对象，习惯使用 `status.OK()` 快速判断访问成功或失败状态，然后再进行下一步处理。

| 类/对象   | 接口名称                         | 功能              | 参数   | 返回值                      |
|--------|------------------------------|-----------------|------|--------------------------|
| Status | bool Status::OK()            | 调用接口是否成功        | void | bool, true: 成功，false: 出错 |
| Status | std::string ToString() const | 以字符格式返回错误码、错误消息 | void | std::string              |

### HTTP Version

tRPC 当前支持 `HTTP/1.1` 和 `HTTP 1.0`， 默认使用 `HTTP/1.1`。暂未支持 `HTTP/2.0`，在准备中。

如果使用使用 1.0 ，可以使用如下方法设置。

```cpp
request.SetVersion("1.0");
```

### Host 字段

tRPC 在解析 URL 时，会将 URL 中的 `host:port` 字段，设置进 HTTP 请求头部中。

提示：
URL 中的 `host` 字段（Domain）并未用作 DNS 查询，或者 `host:port` 字段（IP:Port） 未用作 TCP 创建连接使用，仅用做 HTTP 协议填充使用。

限制：单个 `HttpServiceProxy` 对象实例不支持访问动态 URL。比如，不支持 HTTP 代理转发场景下需要动态访问不同站点。
解决办法：需要针对不同站点创建不同 HttpServiceProxy 实例。

HTTP Client 选择目标 IP:Port 是通过 `target: ip:port` 和 `selector: $naming` 两个配置项来选路的。

常用方式：

* 直连方式。`selector: direct`， 配置单个 IP:Port ：`target: ip:port`，或者 IP:Port
  列表 `target: ip:port,ip:port,ip:port,...`。
* DNS 解析。`selector: domain`，`target: github.com:80` （提示：此处的 target 是 IP:Port 格式，需要补充具体端口）。
* 其他名字服务。`selector: $naming_plugin`， `target: $name`。

那 URL 参数如何填写？
一般情况下，URL 按规范方式填写即可。其中 "host:port"，可以采取占位符方式填写，"host" 和 "port" 可以填写成你期望发送给服务器端的值。
比如：

* 填写 `xx.example.com`，完整 URL：`http://xx.example.com/to/path` ，则 HTTP 请求头部会填充：`Host: xx.example.com`。
* 填写 `xx.example.com:8080`， 完整 URL：`http://xx.example.com:8080/to/path` ， 则 HTTP
  请求头部会填充：`Host: xx.example.com:8080`。

### 设置/获取 HTTP 请求头

Get/Post 接口中并没有直接提供设置 HTTP Request Header 入口，如果要设置请求头，可通过 ClientContext 提供的接口来设置。

| 类/对象          | 接口名称                                                                   | 功能               | 参数                | 返回值                                             |
|---------------|------------------------------------------------------------------------|------------------|-------------------|-------------------------------------------------|
| ClientContext | void SetHttpHeader(const std::string&amp; h, const std::string&amp; v) | 设置 HTTP 请求头      | h: 请求头名称<br>请求头取值 | void                                            |
| ClientContext | const std::string&amp; GetHttpHeader(const std::string&amp; h)         | 获取指定 HTTP 请求头对应值 | h: 请求头名称          | const std::string&amp;                          |
| ClientContext | const auto &amp;GetHttpHeaders();                                      | 获取 HTTP 请求头列表    | void              | google::protobuf::Map<std::string, std::string> |

## 进阶用法

### 发送和接收完整 HTTP 请求和响应

在前面的章节中提到的 `Get Post` 等接口方法中一般是传递 `Client Context` + `URL` + `Request Body` 等参数，没有提供请求头部相关参数。

如果需要：

* 设置请求对象更多参数
* 获取响应对象更多参数

推荐使用 `HttpUnaryInvoke` 接口，可以传递完整的请求对象，或者获取完整的响应对象。

```cpp
Status HttpUnaryInvoke(const ClientContextPtr& context, const HttpRequest& req, HttpRespnose* rsp);
Future<HttpResponse> AsyncHttpUnaryInvoke(const ClientContextPtr& context, const HttpRequest& req);
```

使用这个接口需要调用者完成如下操作：

* 设置 `HTTP Method`。
* 设置 `URL` （相对路径的 Request-URI，比如："/hello?name=tom"）。
* 建议设置 `Host` 头部（某些反向代理服务器可能需要此值做转发，比如 nginx）。

举个例子：

```cpp
bool HttpPost(const HttpServiceProxyPtr& proxy) {
  auto ctx = ::trpc::MakeClientContext(proxy);
  ::trpc::http::Request http_req;
  http_req.SetMethod("POST");
  http_req.SetUrl("/foo");

  std::string req_content = R"({"msg": "hello world!"})";
  http_req.SetHeader("Content-Type", "application/json");
  http_req.SetHeader("Content-Length", std::to_string(req_content.size()));
  http_req.SetContent(req_content);

  ::trpc::http::Response http_rsp;
  auto status = proxy->HttpUnaryInvoke<::trpc::http::Request, ::trpc::http::Response>(ctx, http_req, &http_rsp);
  if (!status.OK()) {
    TRPC_FMT_ERROR("status: {}", status.ToString());
    return false;
  }
  // ...
  return true;
}
```

### 压缩或者解压缩消息体

tRPC-Cpp 当前不会自动压缩或者解压缩消息体，主要基于如下考虑：

* 通用性，这个操作交由用户自行处理会更灵活。
* 压缩、解压缩代码不是很复杂，tRPC 提供了压缩和解压缩工具，当前支持 gzip，lz4， snappy，zlib
  等 [compressor](../../trpc/compressor)

### 发送 HTTPS 请求

HTTPS 是 HTTP over SSL 的简称，可以通过如下方式开启 SSL。

* 编译代码时开启 SSL 编译选项。

  > 使用 bazel build 时，增加 --define trpc_include_ssl=true 编译参数。
  > 提示：也可以加到 .bazelrc 文件中。
  
  **提示：tRPC-Cpp 基于 OpenSSL 支持 HTTPS，请确保编译、运行环境正确安装 OpenSSL 。**

  ```cpp
  // e.g. 
  bazel build --define trpc_include_ssl=true //https/http_client/...
  ```

* 在 `client` 配置中，`service` 配置项中新增 `ssl` 配置项，具体配置如下：
  
  | 名称               | 功能                                        | 取值范围                                    | 默认值               | 可选状态       | 说明                                                              |
  |------------------|-------------------------------------------|-----------------------------------------|-------------------|------------|-----------------------------------------------------------------|
  | ciphers          | 加密套件                                      | 不限                                      | null              | `required` | 在启用SSL时，如果不正确设置，服务会启动失败                                         |
  | enable           | 是否启用SSL                                   | {true, false}                           | false             | optional   | 建议在配置项明确指定，明确意图                                                 |
  | sni_name         | 设置 SSL 协议中 SNI 扩展字段，也可简单理解为在 SSL 中设置 Host | 不限                                      | null              | optional   | 建议设置成 URL 中 Host 值，一般可简单理解为 HTTP 中 Host 值，即域名                   |
  | ca_cert_path     | CA 证书路径                                   | 不限，xx/path/to/ca.pem                    | null              | optional   | 在自签证书场景使用较多                                                     |
  | cert_path        | 证书路径                                      | 不限，xx/path/to/server.pem                | null              | optional   | 双向认证必选，其他情况无效                                                   |
  | private_key_path | 私钥路径                                      | 不限，xx/path/to/server.key                | null              | optional   | 双向认证必选，其他情况无效                                                   |
  | protocols        | SSL协议版本                                   | {SSLv2, SSLv3, TLSv1, TLSv1.1, TLSv1.2} | TLSv1.1 + TLSv1.2 | optional   | -                                                               |
  | insecure         | 是否校验对方证书合法性                               | {true, false}                           | false             | optional   | 默认校验对方证书合法性。在调试场景中，一般使用自签证书，证书不一定能够通过校验，将此参数设置成 true 可以跳过证书校验环节 |
  
  举个例子：
  
  ```yaml
  # @file: trpc_cpp.yaml
  # ...
  client:
    service:
      - name: http_client
        selector_name: direct
        target: 127.0.0.1:24756
        protocol: http
        network: tcp
        conn_type: long
        ## <-- 新增的 SSL 配置项
        ssl:
          enable: true # 可选参数（默认为 false ，禁用 SSL）
          ciphers: HIGH:!aNULL:!kRSA:!SRP:!PSK:!CAMELLIA:!RC4:!MD5:!DSS # 必选参数
          # 下面可选参数按需配置，这里简要列举下
          # sni_name: www.xxops.com # 可选参数
          # ca_cert_path: ./https/cert/xxops-com-chain.pem # 可选参数
          # cert_path: xx_cert.pem # 可选参数，双向认证必选
          # private_key_path: xx_key.pem # 可选参数，双向认证必选
          # insecure: true # 可选参数（默认为 false，禁用非安全模式）
          # protocols: # 可选参数
          #   - SSLv2
          #   - SSLv3
          #   - TLSv1
          #   - TLSv1.1
          #   - TLSv1.2
          ## --> 新增的SSL配置项
  # ...
  ```

### 获取非 2xx 响应的响应内容

tRPC-Cpp 对 HTTP 响应码做了过滤处理：

* 响应码为 2xx 时，调用者可以获取对应的响应消息。
* 响应码非 2xx 时，调用者仅可以获取到响应码，但无响应消息。

解决办法：override `CheckResponse(...)` 直接返回 `true` 即可，举个例子：

```cpp
class MyHttpServiceProxy : public ::trpc::http::HttpSeriveProxy {
  public:
   bool CheckHttpResponse(const ClientContextPtr& context, const ProtocolPtr& http_rsp) override { return true; }
};
```

### 大文件上传 + 下载

在 HTTP 服务中，有部分场景需要读取或者发送大文件的场景，将文件完整读入内存压力较大且效率较低，对于大文件的上传可行性不高。
tRPC-Cpp 提供一套 HTTP 流式读取/写入数据分片的接口，可以分片接收/发送大文件。

* 对于已经知晓长度的大文件，设置 `Content-Length: $length` 后分块发送（当然也可以使用 chunked 分块发送，如果对端支持的话）。
* 对于未知长度的大文件，设置 `Transfer-Encoding: chunked` 后分块发送。

具体可以参考 [HTTP 上传 下载](./http_protocol_upload_download_service.md)。

# 访问 HTTP RPC 服务

示例：[client.cc](../../examples/features/http_rpc/client/client.cc)

如果某个 tRPC RPC 服务在服务端支持 `HTTP` 协议的客户端访问，那客户端也可以使用 `HTTP` 协议访问这个 RPC 服务。

具体方法：

* 调用 `UnaryInvoke(...)`。
* 设置配置项 `protocol: trpc_over_http`。

```cpp
// 模版参数为 protobuf message 对象。
template <class RequestMessage, class ResponseMessage>
Status UnaryInvoke(const ClientContextPtr& context, const RequestMessage& req, ResponseMessage* rsp);
template <class RequestMessage, class ResponseMessage>
Future<ResponseMessage> AsyncUnaryInvoke(const ClientContextPtr& context, const RequestMessage& req);
```

举个例子：

```cpp
bool HttpRpcCall(const HttpServiceProxyPtr& proxy) {
  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(5000);
  // 设置被调用的 RPC 方法名称。
  ctx->SetFuncName("/trpc.test.helloworld.Greeter/SayHello");
  ::trpc::test::helloworld::HelloRequest req;
  ::trpc::test::helloworld::HelloReply rsp;

  req.set_msg("hello world!");
  auto status =
      proxy->UnaryInvoke<::trpc::test::helloworld::HelloRequest, ::trpc::test::helloworld::HelloReply>(ctx, req, &rsp);
  if (!status.OK()) {
    TRPC_FMT_ERROR("status: {}", status.ToString());
    return false;
  }
  // ...
  return true;
}
```

## 设置 `trpc-trans-info`

如果服务端希望使用 HTTP 协议透传 tRPC 协议 `trans_info`，客户端如何处理 ？
设置方法：把 `trpc-trans-info: {"k1": "v1", "k2": "v2"}` 当做一个 HTTP Header，
其中 HTTP Header 名称为：`trpc-trans-info`  ， 值为 `{"k1": "v1", "k2": "v2"}` 是一个 JSON 串。
并使用 `ClientContext::SetHttpHeader('trpc-trans-info', 'json_string')` 设置。

和如下 curl 命令行类似：

```sh
curl -H 'trpc-trans-info: {"k1": "v1", "k2": "v2" }'  -T xx.seriealized.pb $url
```

如果待透传 Key-Value Pairs 包含二进制数据，如何处理？
方法：可以尝试将二进制数据进行 Base64 Decode/Encode 处理，客户端和服务端约定好，保证数据可以正确传递。

# FAQ

## 如何获取 HTTP 响应状态码，比如 200，404？

如果只需要获取 2xx 的状态码，可直接使用返回为 `HttpResponse*` 的接口。
如果需要获取非 2xx 的状态码，请 override `CheckHttpResponse(...)` 方法。

## 配置文件中的 `target` 配置项是否支持 `域名:Port` 格式？

支持的，需要：

* `target` 设置成 `xx.example.com:8080` 。
* 将 `selector_name` 设置成 `domain`。

## 同步接口执行过程中是阻塞线程？

* 如果使用了 `fiber` 协程，同步接口执行过程是同步调用，异步执行，不阻塞线程。
* 如果使用了 `merge or separate` 线程模型，同步接口调用会阻塞调用者线程，但是网络相关操作是异步执行的。

## 如何使用 `curl` 命令发送 Protobuf 数据给 HTTP Service？

参考如下命令（替换你自己的数据 + IP:Port + RPC 方法名）：

```sh
## 其中 http_rpc_hello_request.pb 是 PB 消息序列化后的内容。
curl -T http_rpc_hello_request.pb -H "Content-Type:application/pb" 'http://127.0.0.1:24756/trpc.test.httpserver.Greeter/SayHello'

## 发送 JSON 数据
curl -T http_rpc_hello_request.json -H "Content-Type:application/json" 'http://127.0.0.1:24756/trpc.test.httpserver.Greeter/SayHello'
```

## 使用自签证书调试时，客户端校验证书不通过，如何处理？

ssl 配置项：`insecure` ：是否校验对方证书合法性，默认校验对方证书合法性。在调试场景中，使用自签证书时，可以将此参数设置成 true
可以跳过证书校验环节。

## 调用 HttpUnaryInvoke 等接口时，提示错误：`... err=unmatched codec ...`，如何处理？

尝试设置 HttpServiceProxy 使用的 codec 配置项：`procotol: http`。

## 通过 HTTP 调用 tRPC RPC 服务时，被调方通过 @alias 设置了一个别名如何设置 URL Path ？

方法：client_context->SetFuncName("${your-alias-name}") ，替换下参数值。
