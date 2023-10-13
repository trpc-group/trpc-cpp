[English](../en/trpc_status_code.md)

# 前言

tRPC 框架定义了统一的接口调用返回码，并做了分类。

- 区分服务端和客户端的错误码。
- 区分一问一答调用和流式调用的错误码。
- 错误码细分为网络相关、协议相关、数据操作相关等不同类别。

[错误码定义](../../trpc/proto/trpc.proto)

# 一问一答调用错误码

## 服务端错误码

| 错误码                                    | 含义                                      |
|:---------------------------------------|-----------------------------------------|
| TRPC_INVOKE_SUCCESS = 0                | 调用成功                                    |
| TRPC_SERVER_DECODE_ERR = 1             | 服务端解码错误                                 |
| TRPC_SERVER_ENCODE_ERR = 2             | 服务端编码错误                                 |
| TRPC_SERVER_NOSERVICE_ERR = 11         | 服务端没有相应的服务实现                            |
| TRPC_SERVER_NOFUNC_ERR = 12            | 服务端没有相应的接口实现                            |
| TRPC_SERVER_TIMEOUT_ERR = 21           | 请求在服务端超时                                |
| TRPC_SERVER_OVERLOAD_ERR = 22          | 请求在服务端被过载保护而丢弃请求（主要用在框架内部实现的过载保护插件上）    |
| TRPC_SERVER_LIMITED_ERR = 23           | 请求在服务端被限流（主要用在外部服务治理系统的插件或者业务自定义的限流插件上） |
| TRPC_SERVER_FULL_LINK_TIMEOUT_ERR = 24 | 请求在服务端因全链路超时时间而超时                       |
| TRPC_SERVER_SYSTEM_ERR = 31            | 服务端系统错误                                 |
| TRPC_SERVER_AUTH_ERR = 41              | 服务端鉴权失败错误                               |
| TRPC_SERVER_VALIDATE_ERR = 51          | 服务端请求参数自动校验失败错误                         |

## 客户端错误码

| 错误码                                     | 含义                                      |
|:----------------------------------------|-----------------------------------------|
| TRPC_CLIENT_INVOKE_TIMEOUT_ERR = 101    | 客户端调用超时                                 |
| TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR = 102 | 请求在客户端因全链路超时时间而超时                       |
| TRPC_CLIENT_CONNECT_ERR = 111           | 客户端连接错误                                 |
| TRPC_CLIENT_ENCODE_ERR = 121            | 客户端编码错误                                 |
| TRPC_CLIENT_DECODE_ERR = 122            | 客户端解码错误                                 |
| TRPC_CLIENT_LIMITED_ERR = 123           | 请求在客户端被限流（主要用在外部服务治理系统的插件或者业务自定义的限流插件上） |
| TRPC_CLIENT_OVERLOAD_ERR = 124          | 请求在客户端被过载保护而丢弃请求（主要用在框架内部实现的过载保护插件上）    |
| TRPC_CLIENT_ROUTER_ERR = 131            | 客户端选 IP 路由错误                            |
| TRPC_CLIENT_NETWORK_ERR = 141           | 客户端网络错误                                 |
| TRPC_CLIENT_VALIDATE_ERR = 151          | 客户端响应参数校验失败错误                           |
| TRPC_CLIENT_CANCELED_ERR = 161          | 上游主动断开连接，提前取消请求错误                       |

如下错误发生时，请求包未发送到服务端。

- TRPC_CLIENT_CONNECT_ERR
- TRPC_CLIENT_ENCODE_ERR
- TRPC_CLIENT_LIMITED_ERR
- TRPC_CLIENT_OVERLOAD_ERR
- TRPC_CLIENT_ROUTER_ERR

如下错误发生时，不确定请求包是否发送到服务端。

- TRPC_CLIENT_INVOKE_TIMEOUT_ERR
- TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR
- TRPC_CLIENT_NETWORK_ERR

如下错误时，表示服务端的回包格式有误。

- TRPC_CLIENT_DECODE_ERR

如下错误码目前未使用到。

- TRPC_CLIENT_VALIDATE_ERR
- TRPC_CLIENT_CANCELED_ERR

## 未定义错误

TRPC_INVOKE_UNKNOWN_ERR = 999

# 流式调用错误码

## 服务端流错误码

| 错误码                                           | 含义                   |
|:----------------------------------------------|----------------------|
| TRPC_STREAM_SERVER_NETWORK_ERR = 201          | 服务端流式网络错误            |
| TRPC_STREAM_SERVER_MSG_EXCEED_LIMIT_ERR = 211 | 服务端流式传输错误。比如，流式消息过长。 |
| TRPC_STREAM_SERVER_ENCODE_ERR = 221           | 服务端流式编码错误            |
| TRPC_STREAM_SERVER_DECODE_ERR = 222           | 服务端流式解码错误            |
| TRPC_STREAM_SERVER_WRITE_END = 231            | 服务端流式写流结束            |
| TRPC_STREAM_SERVER_WRITE_OVERFLOW_ERR = 232   | 服务端流式写溢出错误           |
| TRPC_STREAM_SERVER_WRITE_CLOSE_ERR = 233      | 服务端流式写关闭错误           |
| TRPC_STREAM_SERVER_WRITE_TIMEOUT_ERR = 234    | 服务端流式写超时错误           |
| TRPC_STREAM_SERVER_READ_END = 251             | 服务端流式读结束             |
| TRPC_STREAM_SERVER_READ_CLOSE_ERR = 252       | 服务端流式读关闭错误           |
| TRPC_STREAM_SERVER_READ_EMPTY_ERR = 253       | 服务端流式读空错误            |
| TRPC_STREAM_SERVER_READ_TIMEOUT_ERR = 254     | 服务端流式读超时错误           |

## 客户端流错误码

| 错误码                                           | 含义                   |
|:----------------------------------------------|----------------------|
| TRPC_STREAM_CLIENT_NETWORK_ERR = 301          | 客户端流式网络错误            |
| TRPC_STREAM_CLIENT_MSG_EXCEED_LIMIT_ERR = 311 | 客户端流式传输错误。比如，流式消息过长。 |
| TRPC_STREAM_CLIENT_ENCODE_ERR = 321           | 客户端流式编码错误            |
| TRPC_STREAM_CLIENT_DECODE_ERR = 322           | 客户端流式解码错误            |
| TRPC_STREAM_CLIENT_WRITE_END = 331            | 客户端流式写流结束            |
| TRPC_STREAM_CLIENT_WRITE_OVERFLOW_ERR = 332   | 客户端流式写溢出错误           |
| TRPC_STREAM_CLIENT_WRITE_CLOSE_ERR = 333      | 客户端流式写关闭错误           |
| TRPC_STREAM_CLIENT_WRITE_TIMEOUT_ERR = 334    | 客户端流式写超时错误           |
| TRPC_STREAM_CLIENT_READ_END = 351             | 客户端流式读结束             |
| TRPC_STREAM_CLIENT_READ_CLOSE_ERR = 352       | 客户端流式读关闭错误           |
| TRPC_STREAM_CLIENT_READ_EMPTY_ERR = 353       | 客户端流式读空错误            |
| TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR = 354     | 客户端流式读超时错误           |

## 未定义错误

TRPC_STREAM_UNKNOWN_ERR = 1000
