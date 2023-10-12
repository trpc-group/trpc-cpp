[中文](../zh/http_protocol_client.md)

# Overview

tRPC-Cpp supports tRPC protocol by default, and also supports HTTP protocol. Users can send Plain Text or JSON data in
HTTP requests to access standard HTTP services, or send protobuf data in requests to access RPC services. This
allows an RPC service to support both tRPC and HTTP protocols at the same time, and currently supports accessing tRPC
services through HTTP clients.

This article introduces how to access HTTP services based on tRPC-Cpp (referred to as tRPC below), and developers can
learn the following:

* Accessing standard HTTP services
  * Quick start: Use an HTTP client to access HTTP services.
  * Basic usage: Common interfaces for handling requests and responses.
  * Advanced usage: Send HTTPS requests; request message compression, response message decompression; large file
      upload and download.
* Accessing HTTP RPC services
* FAQ

# Accessing standard HTTP services

## Quick start

Before getting started, you can try running the sample code to get a hands-on experience of how an HTTP service works.

### Experience an HTTP service

Example: [http](../../examples/features/http)

Go to the main directory of the tRPC code repository and run the following command.

```shell
sh examples/features/http/run.sh
```

The content of the output from the client program is as follows:

``` text
# Here are some excerpts from the output of the process:
response content: hello world!
response content: {"msg":"hello world!"}
response content: {"msg": "hello world!", "age": 18, "height": 180}

# Test case execution status (1: success, 0: failure)
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

Here's a quick look at the client's output: it sends `GET, HEAD, POST` requests and receives their corresponding
responses. In addition, a Unary RPC was successfully executed.

Next, let's take a look at the steps to access an HTTP service.

### Basic steps to access an HTTP service

1. Get the `HttpServiceProxyPtr` object `proxy`: use `GetClient()->GetProxy<HttpServiceProxy>(...)`.
2. Create the `ClientContextPtr` object `context`: use `MakeClientContext(proxy)`.
3. Call the expected API to access and check the return code, such as `GetString` or `PostString` (we can choose to use
   synchronous or asynchronous interfaces based on your business scenario).

Below is an example of accessing `GetString` using coroutines.

Example: [http_client.cc](../../examples/features/http/client/client.cc)

### Access process

1. Get the `HttpServiceProxyPtr` object `proxy`: use `GetTrpcClient()->GetProxy<HttpServiceProxy>(...)`

    ```cpp
    std::string service_name{"http_client_xx"};
    ::trpc::ServiceProxyOption option;
    
    // ... The initialization process of option is omitted here.
    // The proxy obtained here can be used elsewhere without having to obtain it every time.
    auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(servie_name, option);
    ```
  
    > There are two ways to create an `HttpServiceProxyPtr`:
    >
    > -1- Set the `ServiceProxyOption` object.
    >
    > -2- Use a YAML configuration file to specify the service configuration item. (Recommended)
    >
    > -3- `ServiceProxyOption` initialized by callback defined by user + YAML configuration.
    >
    > Reference to [client_guide](./client_guide.md)

2. Create the `ClientContextPtr` object `context`: use `MakeClientContext(proxy)`.

    ```cpp
      auto ctx = ::trpc::MakeClientContext(proxy);
    ```

3. Invoke `GetString`

    ```cpp
    // ...
    auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(servie_name, option);
    auto ctx = ::trpc::MakeClientContext(proxy);
    // rsp_str stores the body of response.
    std::string rsp_str;
    auto status = proxy->GetString(ctx, "http://example.com/foo", &rsp_str);
    if (!status.OK()) {
      // Error ...
    } else {
      // Ok ...
    }
    // ...
    ```

## Basic usage

`HttpServiceProxy` provides interfaces such as `Head Get Post Put Delete Options`, and for convenience, it also
encapsulates some interfaces for obtaining specific objects, such as commonly used `String` and `JSON` objects.

These interfaces are generally suitable for simple scenarios, such as GET or POST a String or JSON.

For different programming models, synchronous and asynchronous interfaces are provided. Synchronous interfaces are
recommended for `fiber` coroutine mode, and asynchronous interfaces are recommended for `merge or seprate` thread model.

Below is a quick overview of the interface list.

### HttpServiceProxy interface list

Regarding interface naming issues: some interface names may not be elegant and may increase understanding costs. The
documentation will try to clarify as much as possible.

#### GET

The Get interface obtains a JSON/string from the server and provides synchronous and asynchronous calling interfaces.

Note: The Get2 below is just an interface name and does not use the HTTP2 protocol.

| Class/Object     | Interface Name                                                                                           | Functionality                                              | Parameters                                                                               | Return Value                   | Remarks                |
|------------------|----------------------------------------------------------------------------------------------------------|------------------------------------------------------------|------------------------------------------------------------------------------------------|--------------------------------|------------------------|
| HttpServiceProxy | Status Get(const ClientContextPtr&amp; context, const std::string&amp; url, rapidjson::Document* json)   | Obtains a JSON response message using GET                  | client context<br> url resource location <br> json stores the response message   | Status                         | Synchronous interface  |
| HttpServiceProxy | Status GetString(const ClientContextPtr&amp; context, const std::string&amp; url, std::string* rspStr)   | Obtains a string response message using GET                | client context<br> url resource location <br> rspStr stores the response message | Status                         | Synchronous interface  |
| HttpServiceProxy | Status Get2(const ClientContextPtr&amp; context, const std::string&amp; url, HttpResponse* rsp)          | Obtains an HTTP response using GET                         | client context<br> url resource location <br> rsp stores the HTTP response       | Status                         | Synchronous interface  |
| HttpServiceProxy | Future&lt;rapidjson::Document> AsyncGet(const ClientContextPtr&amp; context, const std::string&amp; url) | Obtains a JSON response message using GET asynchronously   | client context<br> url resource location                                         | Future&lt;rapidjson::Document> | Asynchronous interface |
| HttpServiceProxy | Future&lt;std::string> AsyncGetString(const ClientContextPtr&amp; context, const std::string&amp; url)   | Obtains a string response message using GET asynchronously | client context<br> url resource location                                         | Future&lt;std::string>         | Asynchronous interface |
| HttpServiceProxy | Future&lt;HttpResponse> AsyncGet2(const ClientContextPtr&amp; context, const std::string&amp; url)          | Obtains an HTTP response using GET asynchronously          | context client context<br> url resource location                                         | Future&lt;HttpResponse>        | Asynchronous interface |

Translation:

#### POST

The Post interface supports sending a JSON/string request to the server and then obtaining a JSON/string response from
the server.

Note: The Post2 below is just an interface name and does not use the HTTP2 protocol.

| Class/Object     | Interface Name                                                                                                                                 | Functionality                                                                                  | Parameters                                                                                                       | Return Value                   | Remarks                                                    |
|------------------|------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------|--------------------------------|------------------------------------------------------------|
| HttpServiceProxy | Status Post(const ClientContextPtr&amp; context, const std::string&amp; url, const rapidjson::Document&amp; data, rapidjson::Document* rsp)    | Sends a JSON request message using POST and obtains a JSON response message                    | context client context<br> url resource location <br> data request message <br> rsp stores the response message  | Status                         | Synchronous interface                                      |
| HttpServiceProxy | Status Post(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp; data, std::string* rsp)                    | Sends a string request message using POST and obtains a string response message                | context client context<br> url resource location <br> data request message <br> rsp stores the response message  | Status                         | Synchronous interface                                      |
| HttpServiceProxy | Status Post(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp;&amp; data, std::string* rsp)               | Sends a string request message using POST and obtains a string response message                | context client context<br> url resource location <br> data request message <br> rsp stores the response message  | Status                         | Synchronous interface, performance optimization interface  |
| HttpServiceProxy | Status Post2(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp; data, HttpResponse* rsp)                  | Sends a string request message using POST and obtains an HTTP response                         | context client context<br> url resource location <br> data request message <br> rsp stores the response message  | Status                         | Synchronous interface                                      |
| HttpServiceProxy | Status Post2(const ClientContextPtr&amp; context, const std::string&amp; url, std::string&amp;&amp; data, HttpResponse* rsp)                   | Sends a string request message using POST and obtains an HTTP response                         | context client context<br> url resource location <br> data request message <br> rsp stores the response message  | Status                         | Synchronous interface, performance optimization interface  |
| HttpServiceProxy | Status Post(const ClientContextPtr&amp; context, const std::string&amp; url, NoncontiguousBuffer&amp;&amp; data, NoncontiguousBuffer* body)    | Sends a string request message using POST and obtains a string response message                | context client context<br> url resource location <br> data request message <br> body stores the response message | Status                         | Synchronous interface, performance optimization interface  |
| HttpServiceProxy | Future&lt;rapidjson::Document> AsyncPost(const ClientContextPtr&amp; context, const std::string&amp; url, const rapidjson::Document&amp; data) | Sends a JSON request message using POST and obtains a JSON response message asynchronously     | context client context<br> url resource location<br> data request message                                        | Future&lt;rapidjson::Document> | Asynchronous interface                                     |
| HttpServiceProxy | Future&lt;std::string> AsyncPost(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp; data)                 | Sends a string request message using POST and obtains a string response message asynchronously | context client context<br> url resource location<br> data request message                                        | Future&lt;std::string>         | Asynchronous interface                                     |
| HttpServiceProxy | Future&lt;std::string> AsyncPost(const ClientContextPtr&amp; context, const std::string&amp; url, std::string&amp;&amp; data)                  | Sends a string request message using POST and obtains a string response message asynchronously | context client context<br> url resource location<br> data request message                                        | Future&lt;std::string>         | Asynchronous interface, performance optimization interface |
| HttpServiceProxy | Future&lt;NoncontiguousBuffer> AsyncPost(const ClientContextPtr&amp; context, const std::string&amp; url, NoncontiguousBuffer&amp;&amp; data)  | Sends a string request message using POST and obtains a string response message asynchronously | context client context<br> url resource location<br> data request message                                        | Future&lt;NoncontiguousBuffer> | Asynchronous interface, performance optimization interface |
| HttpServiceProxy | Future&lt;HttpResponse> AsyncPost2(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp; data)               | Sends a string request message using POST and obtains an HTTP response                         | context client context<br> url resource location<br> data request message                                        | Future&lt;HttpResponse>        | Asynchronous interface                                     |
| HttpServiceProxy | Future&lt;HttpResponse> AsyncPost2(const ClientContextPtr&amp; context, const std::string&amp; url, const std::string&amp;&amp; data)          | Sends a string request message using POST and obtains an HTTP response                         | context client context<br> url resource location<br> data request message                                        | Future&lt;HttpResponse>        | Asynchronous interface, performance optimization interface |

#### HEAD  PUT  OPTIONS  PATCH  DELETE

*Note: The Put2 below is just an interface name and does not use the HTTP2 protocol.*

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
Status Put(const ClientContextPtr& context, const std::string& url, ::trpc::NoncontiguousBuffer&& data,::trpc::NoncontiguousBuffer* body);
/// @brief Puts a string as the request body to HTTP server, then gets the HTTP response.
Status Put2(const ClientContextPtr& context, const std::string& url, const std::string& data, HttpResponse* rsp);
Status Put2(const ClientContextPtr& context, const std::string& url, std::string&& data, HttpResponse* rsp);

/// @brief Sends an HTTP PATCH request with a string as the request body to HTTP server, then gets the HTTP response.
Status Patch(const ClientContextPtr& context, const std::string& url, const std::string& data, HttpResponse* rsp);
Status Patch(const ClientContextPtr& context, const std::string& url, std::string&& data, HttpResponse* rsp);

/// @brief Sends an HTTP DELETE request with a string as the request body to HTTP server, then gets the HTTP response.
Status Delete(const ClientContextPtr& context, const std::string& url, const std::string& data, HttpResponse* rsp);
Status Delete(const ClientContextPtr& context, const std::string& url, std::string&& data, HttpResponse* rsp);

// The corresponding asynchronous interface is as follows:
Future<HttpResponse> AsyncHead(const ClientContextPtr& context, const std::string& url);

Future<HttpResponse> AsyncOptions(const ClientContextPtr& context, const std::string& url);

Future<rapidjson::Document> AsyncPut(const ClientContextPtr& context, const std::string& url, const rapidjson::Document& data);
Future<std::string> AsyncPut(const ClientContextPtr& context, const std::string& url, const std::string& data);
Future<std::string> AsyncPut(const ClientContextPtr& context, const std::string& url, std::string&& data);
Future<::trpc::NoncontiguousBuffer> AsyncPut(const ClientContextPtr& context, const std::string& url, ::trpc::NoncontiguousBuffer&& data);
Future<HttpResponse> AsyncPut2(const ClientContextPtr& context, const std::string& url, const std::string& data);
Future<HttpResponse> AsyncPut2(const ClientContextPtr& context, const std::string& url, std::string&& data);

Future<HttpResponse> AsyncPatch(const ClientContextPtr& context, const std::string& url, const std::string& data);
Future<HttpResponse> AsyncPatch(const ClientContextPtr& context, const std::string& url, std::string&& data);
Future<HttpResponse> AsyncDelete(const ClientContextPtr& context, const std::string& url, const std::string& data);
Future<HttpResponse> AsyncDelete(const ClientContextPtr& context, const std::string& url, std::string&& data);
```

### CONNECT method is not currently supported

tRPC-Cpp has not yet implemented the HTTP CONNECT related logic.

### Status

The synchronous interface provided by `HttpServiceProxy` returns a `Status` object. It is customary to use `status.OK()`
to quickly determine whether the access is successful or failed, and then proceed to the next step of processing.

| Class/Object | Interface Name               | Function                                                | Parameters | Return Value                        |
|--------------|------------------------------|---------------------------------------------------------|------------|-------------------------------------|
| Status       | bool Status::OK()            | Determine if the interface call was successful          | void       | bool, true: success, false: failure |
| Status       | std::string ToString() const | Return error code and error message in character format | void       | std::string                         |

### HTTP version

tRPC currently supports `HTTP/1.1` and `HTTP 1.0`, with `HTTP/1.1` being the default. `HTTP/2.0` is not yet supported,
but is currently being prepared for.

If you want to use `HTTP 1.0`, you can set it using the following method.

```cpp
request.SetVersion("1.0");
```

### Host field

When parsing a URL, tRPC sets the `host:port` field in the URL as the `Host` field in the HTTP request header.

Note:
The `host` field (Domain) in the URL is not used for DNS queries, and the `host:port` field (IP:Port) is not used for
creating TCP connections. They are only used for filling in the HTTP protocol.

Limitation: A single `HttpServiceProxy` object instance does not support accessing dynamic URLs. For example, it does
not support dynamically accessing different sites in an HTTP proxy forwarding scenario.
Solution: Create different HttpServiceProxy instances for different sites.

The HTTP client selects the target IP:Port through two configuration items: `target: ip:port` and `selector: $naming`.

Common methods:

* Direct connection. `selector: direct`, configure a single IP:Port: `target: ip:port`, or a list of IP:
  Ports: `target: ip:port,ip:port,ip:port,...`.
* DNS resolution. `selector: domain`, `target: github.com:80` (Note: the target here is in IP:Port format and needs to
  specify the specific port).
* Other naming services. `selector: $naming_plugin`, `target: $name`.

How to fill in URL parameters?

In general, the URL can be filled in according to the standard format. The "host:port" field can be filled in using
placeholders, and "host" and "port" can be filled in with the values you want to send to the server.
For example:

* Fill in `xx.example.com`, complete URL: `http://xx.example.com/to/path`, and the HTTP request header will be filled in
  with: `Host: xx.example.com`.
* Fill in `xx.example.com:8080`, complete URL: `http://xx.example.com:8080/to/path`, and the HTTP request header will be
  filled in with: `Host: xx.example.com:8080`.

### Set/Get HTTP request header

There is no direct entry to set HTTP Request Header in the Get/Post interface. If you want to set the request header,
you can use the interface provided by ClientContext to set it.

| Class/Object  | Interface Name                                                         | Function                                                         | Parameters                                     | Return Value                                       |
|---------------|------------------------------------------------------------------------|------------------------------------------------------------------|------------------------------------------------|----------------------------------------------------|
| ClientContext | void SetHttpHeader(const std::string&amp; h, const std::string&amp; v) | Set HTTP request header                                          | h: Request header name<br> Request header value | void                                               |
| ClientContext | const std::string&amp; GetHttpHeader(const std::string&amp; h)         | Get the corresponding value of the specified HTTP request header | h: Request header name                         | const std::string&amp;                             |
| ClientContext | const auto &amp;GetHttpHeaders();                                      | Get the list of HTTP request headers                             | void                                           | google::protobuf::Map&lt;std::string, std::string> |

## Advanced usage

### Send and Receive complete HTTP requests and responses

In the interface methods such as `Get Post` mentioned in the previous chapters, generally
only `Client Context` + `URL` + `Request Body` parameters are passed, and no request header related parameters are
provided.

If you need to:

* Set more parameters for the request object.
* Get more parameters for the response object.

It is recommended to use the `HttpUnaryInvoke` interface, which can pass the complete request object or get the complete
response object.

```cpp
Status HttpUnaryInvoke(const ClientContextPtr& context, const HttpRequest& req, HttpRespnose* rsp);
Future<HttpResponse> AsyncHttpUnaryInvoke(const ClientContextPtr& context, const HttpRequest& req);
```

To use this interface, the caller needs to complete the following operations:

* Set the `HTTP Method`.
* Set the `URL` (Request-URI of the relative path, such as: "/hello?name=tom").
* It is recommended to set the `Host` header (some reverse proxy servers may need this value for forwarding, such as
  nginx).

For example:

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

### Compressing/Decompressing message body

tRPC does not automatically compress or decompress message bodies, mainly for the following reasons:

* Flexibility: allowing users to handle this operation themselves will be more flexible.
* Compression and decompression code is not very complex. tRPC provides compression and decompression tools, currently
  supporting gzip, lz4, snappy, zlib, and more. [compressor](../../trpc/compressor)

### Send HTTPS request

HTTPS is short for HTTP over SSL and can be enabled through the following steps:

* Enable SSL compilation options when compiling the code.

  > When using `bazel build`, add the `--define trpc_include_ssl=true` compilation parameter.
  > Note: We can also add it to the `.bazelrc` file.

  **Note: tRPC-Cpp supports HTTPS based on OpenSSL. Please make sure that OpenSSL is correctly installed in the compilation
and runtime environment.**

  ```cpp
  // e.g. 
  bazel build --define trpc_include_ssl=true //https/http_client/...
  ```

* Set SSL-related configurations in the configuration file. The specific configuration items are as follows:

   | Name             | Function                                                                       | Value Range                             | Default Value     | Optional/Required | Description                                                                                                                                                                                                                                                                     |
   |------------------|--------------------------------------------------------------------------------|-----------------------------------------|-------------------|-------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
   | ciphers          | Encryption suite                                                               | Unlimited                               | null              | `required`        | If SSL is enabled but not set correctly, the service will fail to start.                                                                                                                                                                                                        |
   | enable           | Whether to enable SSL                                                          | {true, false}                           | false             | optional          | It is recommended to specify the configuration item explicitly with clear intent.                                                                                                                                                                                               |
   | sni_name         | Set the SNI extension field in the SSL protocol, or simply set the Host in SSL | Unlimited                               | null              | optional          | It is recommended to set it to the Host value in the URL, which is generally understood as the Host value in HTTP, that is, the domain name.                                                                                                                                    |
   | ca_cert_path     | CA certificate path                                                            | Unlimited, xx/path/to/ca.pem            | null              | optional          | Used more in self-signed certificate scenarios.                                                                                                                                                                                                                                 |
   | cert_path        | Certificate path                                                               | Unlimited, xx/path/to/server.pem        | null              | optional          | Required for mutual authentication, invalid in other cases.                                                                                                                                                                                                                     |
   | private_key_path | Private key path                                                               | Unlimited, xx/path/to/server.key        | null              | optional          | Required for mutual authentication, invalid in other cases.                                                                                                                                                                                                                     |
   | protocols        | SSL protocol version                                                           | {SSLv2, SSLv3, TLSv1, TLSv1.1, TLSv1.2} | TLSv1.1 + TLSv1.2 | optional          | -                                                                                                                                                                                                                                                                               |
   | insecure         | Whether to verify the legality of the other party's certificate                | {true, false}                           | false             | optional          | By default, the legality of the other party's certificate is verified. In the debugging scenario, self-signed certificates are generally used, and the certificate may not pass the verification. Setting this parameter to true can skip the certificate verification process. |

  For example：
  
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
        ## <-- New SSL configuration items.
        ssl:
          enable: true # Optional configuration (defaults to false, indicating that SSL is disabled).
          ciphers: HIGH:!aNULL:!kRSA:!SRP:!PSK:!CAMELLIA:!RC4:!MD5:!DSS # Requireda. 
          # sni_name: www.xxops.com # Optional. 
          # ca_cert_path: ./https/cert/xxops-com-chain.pem # Optional. 
          # cert_path: xx_cert.pem # Optional, but it's required for mutual authentication. 
          # private_key_path: xx_key.pem # Optional, but it's required for mutual authentication.
          # insecure: true # Optional. (default to false, disable insecure mode）
          # protocols: # Optional.
          #   - SSLv2
          #   - SSLv3
          #   - TLSv1
          #   - TLSv1.1
          #   - TLSv1.2
          ## --> New SSL configuration items.
  # ...
  ```

### Getting the Response content of Non-2xx responses

tRPC-Cpp has filtered HTTP response codes:

* When the response code is 2xx, the caller can get the corresponding response message.
* When the response code is not 2xx, the caller can only get the response code but not the response message.

Solution: Override `CheckResponse(...)` and return `true` directly. For example:

```cpp
class MyHttpServiceProxy : public ::trpc::http::HttpSeriveProxy {
  public:
   bool CheckHttpResponse(const ClientContextPtr& context, const ProtocolPtr& http_rsp) override { return true; }
};
```

### Large File Upload + Download

In HTTP services, there are scenarios where large files need to be read or sent. Reading the entire file into memory is
inefficient and can cause high memory pressure, making it impractical for uploading large files. tRPC-Cpp provides a set of
HTTP stream reading/writing data chunk interfaces that can be used to receive/send large files in chunks.

* For large files with known length, set `Content-Length: $length` and send them in chunks (or use chunked transfer
  encoding if the recipient supports it).
* For large files with unknown length, set `Transfer-Encoding: chunked` and send them in chunks.

For more details, please refer to the documentation [http upload download](./http_protocol_upload_download_service.md).

# Accessing HTTP RPC services

Example: [client.cc](../../examples/features/http_rpc/client/client.cc)

If a tRPC RPC service supports client access using the `HTTP` protocol on the server side, the client can also access
this RPC service using the `HTTP` protocol.

* Call `UnaryInvoke(...)`.
* Set the configuration item `protocol: trpc_over_http`.

```cpp
// Template parameters: protobuf Message.
template <class RequestMessage, class ResponseMessage>
Status UnaryInvoke(const ClientContextPtr& context, const RequestMessage& req, ResponseMessage* rsp);
template <class RequestMessage, class ResponseMessage>
Future<ResponseMessage> AsyncUnaryInvoke(const ClientContextPtr& context, const RequestMessage& req);
```

For example：

```cpp
bool HttpRpcCall(const HttpServiceProxyPtr& proxy) {
  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(5000);
  // Set RPC function name.
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

## Set `trpc-trans-info`

If the server wants to pass the tRPC protocol `trans_info` through the HTTP protocol, how does the client handle it?

Treat `trpc-trans-info: {"k1": "v1", "k2": "v2"}` as an HTTP header, where the HTTP header name
is `trpc-trans-info` and the value `{"k1": "v1", "k2": "v2"}` is a JSON string.
Use `ClientContext::SetHttpHeader('trpc-trans-info', 'json_string')` to set it.

Similar to the following curl command line:

```sh
curl -H 'trpc-trans-info: {"k1": "v1", "k2": "v2" }'  -T xx.seriealized.pb $url
```

If the Key-Value Pairs to be passed contain binary data, how should they be handled?

We can try to Base64 decode/encode the binary data, and the client and server agree to ensure that the data can
be transmitted correctly.

# FAQ

## How to get the HTTP response status code, such as 200, 404?

If you only need to get the status code of 2xx, you can use the interface that returns `HttpResponse*`.
If you need to get the status code of non-2xx, please override the `CheckHttpResponse(...)` method.

## Does the `target` configuration item in the configuration file support the `domain:Port` format?

Yes, it is supported. You need to:

* Set `target` to `xx.example.com:8080`.
* Set `selector_name` to `domain`.

## Is the thread blocked during the execution of the synchronous interface?

* If the `fiber` coroutine is used, the synchronous interface execution process is a synchronous call, which is executed
  asynchronously and does not block the thread.
* If the `merge or separate` thread model is used, the synchronous interface call will block the caller thread, but the
  network-related operations are executed asynchronously.

## How to use the `curl` command to send Protobuf data to an HTTP service?

Refer to the following command (replace your own data + IP:Port + RPC method name):

```sh
## http_rpc_hello_request.pb is the serialized content of the PB message.
curl -T http_rpc_hello_request.pb -H "Content-Type:application/pb" 'http://127.0.0.1:24756/trpc.test.httpserver.Greeter/SayHello'

## Send JSON data
curl -T http_rpc_hello_request.json -H "Content-Type:application/json" 'http://127.0.0.1:24756/trpc.test.httpserver.Greeter/SayHello'
```

## When using a self-signed certificate for debugging, how to handle the client certificate verification failure?

ssl configuration item: `insecure`: whether to verify the legality of the other party's certificate. By default, the
legality of the other party's certificate is verified. In the debugging scenario, when using a self-signed certificate,
you can set this parameter to true to skip the certificate verification process.

## When calling interfaces such as `HttpUnaryInvoke`, an error is prompted: `... err=unmatched codec ...`, how to handle it?

Try to set the codec configuration item used by HttpServiceProxy: `protocol: http`.

## When calling an tRPC RPC service through HTTP, if the called party sets an alias through @alias, how to set the URL Path?

Replace the parameter value: `client_context->SetFuncName("${your-alias-name}")`.
