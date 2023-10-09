[中文](../zh/http_protocol_service.md)

[TOC]

# HTTP Service Development Guide

**Topic: How to develop an HTTP service based on tRPC-Cpp**

Currently, tRPC-Cpp supports two types of HTTP services: HTTP Standard Service and HTTP RPC Service (these names are
used to
differentiate between the two services).

> HTTP standard service: It is a regular HTTP service that does not use proto files to define the service interface.
> Users need to write their own code to define the service interface and register URL routes.
>
> HTTP RPC service: It is an RPC service where clients can access it using the HTTP protocol. The service invocation
> interface is defined by proto files, and stub code can be generated using tools.


This article introduces how to develop an HTTP service based on tRPC-Cpp (referred to as tRPC below). Developers can
learn the
following:

* How to develop an HTTP standard service:
    * Quick start: Set up an HTTP service quickly.
    * Feature overview: RESTful, HTTPS.
    * Basic usage: Common interfaces for handling requests and responses, configuring request routing rules.
    * Advanced usage: Grouping routing rules, HTTPS, message compression and decompression, file upload and download,
      etc.
* How to develop an HTTP RPC service:
    * Quick start: Support HTTP client access to tRPC services.
* FAQ.

# Developing an HTTP standard service

## Quick start

Before getting started, you can try running the sample code to get a hands-on experience of how an HTTP service works.

### Experience an HTTP Service

Example: [http](../../examples/features/http)

Go to the main directory of the tRPC code repository and run the following command (please be patient as this process
involves downloading, compiling, and starting the code, which may take a little while).

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

### Basic steps for developing an HTTP service

Here's an example of a simple "Hello World" program to illustrate the basic steps for developing an HTTP service.

Example: [http_server.cc](../../examples/features/http/server/http_server.cc).

Basic functionality:

* Support HTTP client to access using methods like `HEAD or GET or POST`.
* For example, a `POST` request with the body  `Hello world!` will receive a response of `Hello world!`.

Basic steps:

1. Implement the `HttpHandler` interface.
2. Set up request routing.
3. Register an instance of `HttpService` with the server.

### Implementation process

1. `FooHandler` implements the `HttpHandler` interface to handle HTTP requests and set the HTTP response content.

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

`HttpHandler` is an interface that provides a simple abstraction for HTTP endpoints. It defines methods corresponding to
common HTTP methods such as GET, POST, PUT, DELETE, etc. Each method in the interface represents a specific HTTP method
that can be handled by the implementing class.

```cpp
class HttpHandler {
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

2. Setting up request routing: Route requests with the URI `/foo` to the `FooHandler` class, supporting the HEAD, GET,
   and POST methods.

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

3. Register an instance of `HttpService` during the application initialization phase and set up request routing.

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

After the application is started, we can access it through the following URLs.

```bash
# GET /foo HTTP1.1
# e.g: curl http://$ip:$port/foo 
$ hello world!
```

## Feature overview

| Feature                         | Support |                                                                           Note |
|:--------------------------------|:-------:|-------------------------------------------------------------------------------:|
| RESTful HEAD/GET/POST ... PATCH |   Yes   |                                                  Provide sync/async interfaces |
| HTTPS                           |   Yes   |              Supports mutual authentication, see SSL configuration for details |
| Compression/Decompression       |   Yes   | Provides tools such as gzip, snappy, lz4, which need to be handled by the user |
| Large file upload/download      |   Yes   |                                                  Provide sync/async interfaces |
| HTTP2                           |   No    |                                                                    Coming soon |
| HTTP3                           |   No    |                                                                              - |

## Basic usage

### HTTP Method

We can obtain the `HTTP Method` through the `GetMethod()` interface.

```cpp
const std::string& GetMethod() const;
```

```cpp
// e.g. log the request method.
TRPC_FMT_INFO("request method: {}", request->GeMethod());
```

If the request line is `GET /hello?a=foo&b=bar HTTP/1.1`, the log will record `request method: GET`.

### Request URI

We can obtain the `Request URI` through the `GetUrl()` interface.

```cpp
const std::string& GetUrl() const;
```

```cpp
// e.g. log the request URI. 
TRPC_FMT_INFO("request uri: {}", request->GetUrl());
```

If the request line is `GET /hello?a=foo&b=bar HTTP/1.1`, the log will record `request uri: /hello?a=foo&b=bar`.

### Query String

`Query String` contains multiple key-value pairs, where the keys are case-sensitive. It is transmitted through
the `Request URI` and commonly takes the form of `key1=value1&key2=value2&...`. It is easy to read and modify, and is
commonly used to pass application-level parameters. We can obtain the parameter object using `GetQueryParameters()`.

```cpp
const QueryParameters& GetQueryParameters() const;
```

```cpp
// e.g. get the `a` and `b` parameters in the Query String.
auto a = GetQueryParameters().Get("a");
auto b = GetQueryParameters().Get("b");
```

If the request line is `GET /hello?a=foo&b=bar HTTP/1.1`, we will get parameters `a= "foo"; b = "bar"`.

### HTTP Headers

Headers consist of multiple key-value pairs, and in the HTTP protocol, the keys are case-insensitive. `Request/Response`
provides interfaces for adding, deleting, modifying, and querying headers.

```cpp
// e.g. get the User-Agent value from the request header.
auto user_agent = request->GetHeader("User-Agent");
// e.g. get the Content-Type value from the request header.
auto content_type = request->GetHeader("Content-Type");

// e.g. set the Content-Type of the response to JSON.
response->SetHeader("Content-Type", "application/json");
```

### HTTP Body

The message body of the request and response is a buffer, which provides two interfaces for access:

* Continuous buffer: `std::string`.
* Non-continuous buffer: `NonContiguousBuffer`.

```cpp
const std::string& GetContent() const;
void SetContent(std::string content);

// Better performance.
const NoncontiguousBuffer& GetNonContiguousBufferContent() const;
void SetNonContiguousBufferContent(NoncontiguousBuffer&& content); 
```

```cpp
// e.g. get the body from the request.
auto& body = request->GetContent();

// e.g. set the body of the response to "Hello world!".
response->SetContent("Hello world!");
```

### Response Status

`status` is a field specific to `Response`, which indicates the completion status of the HTTP response.
The values are defined in [http/status.h](../../trpc/util/http/status.h).

```cpp
int GetStatus() const; 
void SetStatus(int status); 
```

```cpp
// e.g. set the status of the response to redirection. 
response->SetStatus(::trpc::http::ResponseStatus::kFound);
response->SetHeader("Location", "https://github.com/");
```

## Basic routing matching rules

The following matching rules are currently supported:

* Exact match.
* Prefix match.
* Regular expression match.
* Placeholder match.

### Exact match

When filling in the path directly in `Path(path_value)`, exact matching is used by default. That is, the
request will only be processed by the corresponding handler when the path in the request URI exactly matches
the `path_value` set in the routing.

```cpp
 r.Add(trpc::http::MethodType::GET, trpc::http::Path("/hello/img_x"), handler);
```

```bash 
$ curl http://$ip:$port/hello/img_x -X GET  # response status 200
$ curl http://$ip:$port/hello/img -X GET    # response status 404
```

### Prefix match

By appending a path placeholder using `Path(path_value).Remainder("path")`, prefix matching can be used. That is, the
request can be processed by the corresponding handler only when the path prefix in the request URI matches the path.
> The content in `Remainder` is only a placeholder and has no actual meaning. It is recommended to fill it in as `path`
> by default.

```cpp
 r.Add(trpc::http::MethodType::GET, trpc::http::Path("/hello").Remainder("path"), handler_hello);
```

```bash
curl http://$ip:$port/hello/img -X GET    # response status 200
curl http://$ip:$port/hello/img/1 -X GET  # response status 200
curl http://$ip:$port/hello -X GET        # response status 200

# If the prefix does not match, the access fails.
curl http://$ip:$port/hello1 -X GET       # response status 404
curl http://$ip:$port/hello1/img -X GET   # response status 404
```

### Regular expression match

By wrapping `path_value` with `<regex()>`, regular expression matching can be used. That is, the actual path value
filled in is a regular expression, and the request will be processed by the corresponding handler when the path in the
request URI matches the regular expression.

```C++
r.Add(trpc::http::MethodType::GET, trpc::http::Path("<regex(/img/[a-z]_.*)>"), handler);
```

```bash
curl http://$ip:$port/img/d_123/ccd -X GET`    # response status 200
curl http://$ip:$port/img/123_123/ccd -X GET`  # response status 404
```

### Placeholder match

By wrapping `path_value` with `<ph()>`, placeholder matching can be used. That is, the actual path value can contain
placeholders. When the path in the request URI matches the regular expression, the request will be processed by the
corresponding handler. This is particularly useful for path parameters.

The placeholders in the following URI path include `channel_id` and `client_id`:

```cpp
r.Add(trpc::http::MethodType::GET, trpc::http::Path("<ph(/channels/<channel_id>/clients/<client_id>)>"), handler);
```

The values of the placeholders can be easily obtained:

```C++
::trpc::Status Get(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                   ::trpc::http::Response* rsp) override {
  auto channel_id = req->GetParameters().Path("channel_id");
  auto client_id = req->GetParameters().Path("client_id");
  // ...
  return ::trpc::kSuccStatus;
}
```

```bash 
curl http://$ip:$port/channels/xyz/clients/123 -X GET  # response status 200
curl http://$ip:$port/channels/clients -X GET          # response status 404
```

**Note**
> The symbols in the placeholders only support letters, numbers, and underscores.

## Advanced usage

This section introduces advanced usage, such as grouped routing rules, HTTPS, message body compression and
decompression, large file upload and download, and more.

### Grouped routing rules

The `HttpHandlerGroups` utility class provides a more readable and simplified method for registering HTTP routes. It is
suitable for services that require multi-level paths, such as RESTful services.

Currently, `HttpHandlerGroups` only supports two forms of matching: `exact matching` and `placeholder matching`. When
using it, we need to include `trpc/util/http/http_handler_groups.h`.

The new macro commands are as follows. For detailed usage, please refer to the code examples below:

| Macro Name                   | Function                                                                              | Description                                                                                                                                                                             | Parameters                                                                                                          | Examples                                                                              |
|------------------------------|---------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------|
| TRPC_HTTP_ROUTE_HANDLER      | Declare a layer of HTTP RESTful routing, which can be nested                          | By default, it captures all external parameters (such as *global variables, member variables*), and **cannot capture scopes beyond the outermost function** (such as *local variables*) | { Specific routing declaration code }                                                                               | `TRPC_HTTP_ROUTE_HANDLER({ code; })`                                                  |
| TRPC_HTTP_ROUTE_HANDLER_ARGS | Declare a layer of HTTP RESTful routing with captured parameters, which can be nested | By default, it captures all external parameters (such as *global variables, member variables*) and **additional passed-in parameters** (which **!!#ff0000 can!!** be *local variables*) | ( Values of all passed-in parameters ), ( Types and names of each parameter ) { Specific routing declaration code } | `TRPC_HTTP_ROUTE_HANDLER_ARGS((a, 1, a+1), (double a, int b, auto c) { code; })`      |
| TRPC_HTTP_HANDLER            | Bind the specified object and its member method as a routing endpoint                 | -                                                                                                                                                                                       | shared_ptr of the object, name of the member method (fill in as `class name::method name`)                          | `TRPC_HTTP_HANDLER(coffee_pot_controller, CoffeePotController::Brew)`                 |
| TRPC_HTTP_STREAM_HANDLER     | Bind the specified object and its member method as a *streaming* routing endpoint     | -                                                                                                                                                                                       | shared_ptr of the object, name of the member method (fill in as `class name::method name`)                          | `TRPC_HTTP_STREAM_HANDLER(stream_download_controller, StreamDownloadController::Get)` |

Let's first take a look at the complete example:

```cpp
void SetHttpRoutes(trpc::http::HttpRoutes& routes) {
  // It is recommended to add  `clang-format` to avoid undesirable code formatting results.
  // clang-format off
  
  // TRPC_HTTP_ROUTE_HANDLER Declares a layer of HTTP RESTful routing, which can be nested.
  // The default TRPC_HTTP_ROUTE_HANDLER captures all external references, but is limited by the inner class of the 
  // function and cannot capture scopes beyond the outermost function. 
  trpc::http::HttpHandlerGroups(routes).Path("/", TRPC_HTTP_ROUTE_HANDLER({  
    // Each layer of path can be a regular string or a placeholder declared in angle brackets, as shown below. 
    // The prefix and suffix "/" can be ignored.
    Path("/api", TRPC_HTTP_ROUTE_HANDLER({  
      auto hello_world_handler = std::make_shared<ApiHelloWorldHandler>();
      Path("/user", TRPC_HTTP_ROUTE_HANDLER({
        // Get  - /api/user - An `ApiUserHandler` object (which must be default-constructible) will be automatically 
        // registered and managed here. 
        Get<ApiUserHandler>();              
        // Post - /api/user - Reuse the same object of the same type in the same `HttpHandlerGroups`. 
        Post<ApiUserHandler>("/");          
        // Delete - /api/user/<user_id> - Reuse the object - the path placeholder name is `user_id`.
        Delete<ApiUserHandler>("/<user_id>");  
      });
      
      // We can use `TRPC_HTTP_ROUTE_HANDLER_ARGS((captures...), (args...) { ... })` to customize the values to be 
      // captured and the variable names after capturing.
      Path("/hello", TRPC_HTTP_ROUTE_HANDLER_ARGS((hello_world_handler), (auto h) {
        // Head - /api/hello_world - Objects can be registered using `std::shared_ptr`.
        Head("/hello_world", h);  
      });
      // The first matching route takes priority. `/api/teapot` will come here.
      Get("/teapot", [](auto ctx, auto req, auto rep) {                                    
        rep->SetStatus(::trpc::http::ResponseStatus::kIAmATeapot);
      });
      // Whereas `/api/coffee_pot` will come here.
      Get("<path>", TRPC_HTTP_HANDLER(coffee_pot_controller, CoffeePotController::Brew)); 
    });
  });
  // clang-format on
}
```

#### Register `HttpHandler`

Use the `TRPC_HTTP_ROUTE_HANDLER` macro to register `HttpHandler` interface routes.

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
      // Get  - /api/user - An `ApiUserHandler` object (which must be default-constructible) will be automatically 
      // registered and managed here. 
      Get<ApiUserHandler>();      
      // Post - /api/user - Reuse the same object of the same type in the same `HttpHandlerGroups`. 
      Post<ApiUserHandler>("/");
    });
    auto hello_world_handler = std::make_shared<ApiHelloWorldHandler>();
    Head("/hello_world", hello_world_handler);
  });
});
// ...
```

#### Register `HttpController`

Using the `HTTP_HANDLER` macro, users can easily register `Controller`-like interfaces to the route:

Note: `Controller` does not inherit the `HttpHandler` class, but only has the same interface signature.

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

#### Register `Lambda` function

If needed, users can directly register lambda functions to the route.

```cpp
trpc::http::HttpHandlerGroups(routes).Path("/api", TRPC_HTTP_ROUTE_HANDLER({
   Get("/teapot", [](auto ctx, auto req, auto rep) {  
     rep->SetStatus(::trpc::http::ResponseStatus::kIAmATeapot);
   });
   Get("<path>", handler);
});
```

### Compressing/Decompressing message body

In order to reduce page loading time and speed up page rendering, HTTP services may compress response message bodies.
However, tRPC does not automatically compress or decompress message bodies, mainly for the following reasons:

- Flexibility: allowing users to handle this operation themselves will be more flexible.
- Compression and decompression code is not very complex. tRPC provides compression and decompression tools, currently
  supporting gzip, lz4, snappy, zlib, and more. [compressor](../../trpc/compressor)

### Handling HTTPS requests

HTTPS is short for HTTP over SSL and can be enabled through the following steps:

- Enable SSL compilation options when compiling the code.

> When using `bazel build`, add the `--define trpc_include_ssl=true` compilation parameter.
> Note: We can also add it to the `.bazelrc` file.

**Note: tRPC supports HTTPS based on OpenSSL. Please make sure that OpenSSL is correctly installed in the compilation
and runtime environment.**

```cpp
// e.g. 
bazel build --define trpc_include_ssl=true //https/http_server/...
```

- Set SSL-related configurations in the configuration file. The specific configuration items are as follows:

| Name             | Function                                | Range of values                         | Default value     | Optional/Required | Description                                                                               |
|------------------|-----------------------------------------|-----------------------------------------|-------------------|-------------------|-------------------------------------------------------------------------------------------|
| cert_path        | Certificate path                        | Unlimited, xx/path/to/server.pem        | null              | `required`        | If the correct path is not set when SSL is enabled, the service will fail to start.       |
| private_key_path | Private key path                        | Unlimited, xx/path/to/server.key        | null              | `required`        | If the correct path is not set when SSL is enabled, the service will fail to start.       |
| ciphers          | Encryption suite                        | Unlimited                               | null              | `required`        | If not set correctly when SSL is enabled, the service will fail to start.                 |
| enable           | Whether to enable SSL                   | {true, false}                           | false             | optional          | It is recommended to explicitly specify the configuration item to indicate the intention. |
| mutual_auth      | Whether to enable mutual authentication | {true, false}                           | false             | optional          | -                                                                                         |
| ca_cert_path     | CA certificate path                     | Unlimited, xx/path/to/ca.pem            | null              | optional          | Valid when mutual authentication is enabled.                                              |
| protocols        | SSL protocol version                    | {SSLv2, SSLv3, TLSv1, TLSv1.1, TLSv1.2} | TLSv1.1 + TLSv1.2 | optional          | -                                                                                         |

For example:

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
      ## <-- New SSL configuration items.
      ssl:
        enable: true  # Optional configuration (defaults to false, indicating that SSL is disabled).
        cert_path: ./https/cert/server_cert.pem # Required.
        private_key_path: ./https/cert/server_key.pem # Required.
        ciphers: HIGH:!aNULL:!kRSA:!SRP:!PSK:!CAMELLIA:!RC4:!MD5:!DSS # Required.
        # mutual_auth: true # Optional configuration (defaults to false, indicating that mutual authentication is not enabled).
        # ca_cert_path: ./https/cert/xxops-com-chain.pem # Optional configuration, the CA path for mutual authentication.
        # protocols: # Optional.
        #   - SSLv2
        #   - SSLv3
        #   - TLSv1
        #   - TLSv1.1
        #  - TLSv1.2
        ## --> New SSL configuration items.
# ...
```

### Service asynchronously responds to clients

If the server processes the logic of the HTTP request asynchronously and the user expects to reply to the client
actively, instead of letting tRPC automatically reply to the client, the following approach can be adopted:

- Turn off tRPC's response: context->SetResponse(false).
- When replying, serialize the HTTP response into a non-continuous buffer, and then call context->SendResponse(buffer)
  to send the response packet.

```cpp
// ServerContext interface.
void SetResponse(bool is_response);
Status SendResponse(NoncontiguousBuffer&& buffer);

// HTTP Response interface. 
bool SerializeToString(NoncontiguousBuffer& buff) const;
```

The code snippet is as follows:

```cpp
trpc::Status Get(const trpc::ServerContextPtr& context, const trpc::http::RequestPtr req, trpc::http::Response* rep) {
  // ...
  // Turn off tRPC's response.
  context->SetResponse(false);
  // Async job.
  DoAsyncWork(context,req);
  // ...
  return trpc::kSucctStatus;
}

void DoAsyncWork(const trpc::ServerContextPtr& context,const trpc::http::RequestPtr& req){
  // ...
  
  trpc::http::Response http_response;
  http_response.GenerateCommonReply(req.get());

  // Sets the content of response.
  http_response.SetContent("DoAsyncWork");
    
  // Serializes the HTTP response into a non-continuous buffer.
  trpc::NoncontiguousBuffer out;
  http_response.SerializeToString(out);
    
  // Sends response packet. 
  context->SendResponse(std::move(out));
}
```

### Large File Upload + Download

In HTTP services, there are scenarios where large files need to be read or sent. Reading the entire file into memory is
inefficient and can cause high memory pressure, making it impractical for uploading large files. tRPC provides a set of
HTTP stream reading/writing data chunk interfaces that can be used to receive/send large files in chunks.

- For large files with known length, set `Content-Length: $length` and send them in chunks (or use chunked transfer
  encoding if the recipient supports it).
- For large files with unknown length, set `Transfer-Encoding: chunked` and send them in chunks.

For more details, please refer to the documentation [http upload download](./http_protocol_upload_download_service.md).

# Developing HTTP RPC Service

## Quick Start

### Providing tRPC service under HTTP Protocol

In some scenarios, some RPC services may need to support HTTP clients externally. That is, the tRPC protocol header and
payload are transmitted through the HTTP protocol. This can be easily achieved by modifying the configuration items
without modifying the code: `protocol: trpc` -> `protocol: trpc_over_http`.

If we need to support both tRPC and HTTP clients at the same time, we can register two RPC service instances during
the service initialization phase.

The code snippet is as follows:

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

The configuration snippet is as follows:

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

# FAQ

## 1. How to write routing rules to match all paths?

```cpp
// e.g. :
r.Add(trpc::http::MethodType::POST, trpc::http::Path("").Remainder("path"), handler);
```

## 2. Why does the service crash with a coredump after enabling SSL?

Please make sure that the file paths for the certificate, private key, and other files in the SSL
configuration item are configured correctly.

## 3. Why does it take an extra 1 second to upload 1KB+ data using curl?

When uploading data using curl/libcurl, when the body is larger than 1K, it will first ask the server if it allows
sending large data packets (the request header adds "Expect: 100 continue").
If allowed, a 100 continue code is returned, and then libcurl continues to send the request body.
If the server does not respond with 100 continue, after waiting for 1 second, the request body will continue to be sent
to the server.
At present, there is no response of Expect: 100 continue on the tRPC HTTP Server side, so curl need to wait for 1
second before sending the data.

Workaround:

1. When testing with curl, send an empty "Expect:" value, e.g. curl -H "Expect:" -X POST -T "ddd" $url.
2. Use the tRPC HTTP upload and download related interfaces to implement the relevant service interfaces. These
   interfaces support responding with 100 Continue.