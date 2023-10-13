[中文](../zh/trpc_status_code.md)

# Overview

The tRPC framework defines unified error codes and categorizes them:

- Distinguishes between server-side and client-side error codes.
- Distinguishes between request-response calls and streaming calls.
- Error codes are further divided into different categories such as network-related, protocol-related, and data
  operation-related.

[Error code definition](../../trpc/proto/trpc.proto)

# Request-Response Call Error Codes

## Server-Side Error Codes

| Error Code                             | Meaning                                                                                                                                                      |
|:---------------------------------------|:-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| TRPC_INVOKE_SUCCESS = 0                | Call succeeded                                                                                                                                               |
| TRPC_SERVER_DECODE_ERR = 1             | Server-side decoding error                                                                                                                                   |
| TRPC_SERVER_ENCODE_ERR = 2             | Server-side encoding error                                                                                                                                   |
| TRPC_SERVER_NOSERVICE_ERR = 11         | Server-side service implementation not found                                                                                                                 |
| TRPC_SERVER_NOFUNC_ERR = 12            | Server-side interface implementation not found                                                                                                               |
| TRPC_SERVER_TIMEOUT_ERR = 21           | Request timed out on the server-side                                                                                                                         |
| TRPC_SERVER_OVERLOAD_ERR = 22          | Request discarded on the server-side due to overload protection (mainly used in overload protection plugins implemented internally in the framework)         |
| TRPC_SERVER_LIMITED_ERR = 23           | Request limited on the server-side (mainly used in plugins for external service governance systems or custom flow control plugins implemented by businesses) |
| TRPC_SERVER_FULL_LINK_TIMEOUT_ERR = 24 | Request timed out due to full-link timeout on the server-side                                                                                                |
| TRPC_SERVER_SYSTEM_ERR = 31            | Server-side system error                                                                                                                                     |
| TRPC_SERVER_AUTH_ERR = 41              | Server-side authentication failure error                                                                                                                     |
| TRPC_SERVER_VALIDATE_ERR = 51          | Server-side request parameter automatic verification failure error                                                                                           |

## Client-Side Error Codes

| Error Code                              | Meaning                                                                                                                                                      |
|:----------------------------------------|:-------------------------------------------------------------------------------------------------------------------------------------------------------------|
| TRPC_CLIENT_INVOKE_TIMEOUT_ERR = 101    | Client-side call timed out                                                                                                                                   |
| TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR = 102 | Request timed out due to full-link timeout on the client-side                                                                                                |
| TRPC_CLIENT_CONNECT_ERR = 111           | Client-side connection error                                                                                                                                 |
| TRPC_CLIENT_ENCODE_ERR = 121            | Client-side encoding error                                                                                                                                   |
| TRPC_CLIENT_DECODE_ERR = 122            | Client-side decoding error                                                                                                                                   |
| TRPC_CLIENT_LIMITED_ERR = 123           | Request limited on the client-side (mainly used in plugins for external service governance systems or custom flow control plugins implemented by businesses) |
| TRPC_CLIENT_OVERLOAD_ERR = 124          | Request discarded on the client-side due to overload protection (mainly used in overload protection plugins implemented internally in the framework)         |
| TRPC_CLIENT_ROUTER_ERR = 131            | Client-side IP routing error                                                                                                                                 |
| TRPC_CLIENT_NETWORK_ERR = 141           | Client-side network error                                                                                                                                    |
| TRPC_CLIENT_VALIDATE_ERR = 151          | Client-side response parameter verification failure error                                                                                                    |
| TRPC_CLIENT_CANCELED_ERR = 161          | Request canceled error due to upstream actively disconnecting the connection                                                                                 |

When the following errors occur, the request packet is not sent to the server:

- TRPC_CLIENT_CONNECT_ERR
- TRPC_CLIENT_ENCODE_ERR
- TRPC_CLIENT_LIMITED_ERR
- TRPC_CLIENT_OVERLOAD_ERR
- TRPC_CLIENT_ROUTER_ERR

When the following errors occur, it is uncertain whether the request packet is sent to the server:

- TRPC_CLIENT_INVOKE_TIMEOUT_ERR
- TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR
- TRPC_CLIENT_NETWORK_ERR

When the following error occurs, it means that the format of the server-side response packet is incorrect:

- TRPC_CLIENT_DECODE_ERR

The following error codes are currently unused:

- TRPC_CLIENT_VALIDATE_ERR
- TRPC_CLIENT_CANCELED_ERR

## Undefined Errors

TRPC_INVOKE_UNKNOWN_ERR = 999

# Streaming Call Error Codes

## Server-Side Streaming Error Codes

| Error Code                                    | Meaning                                                                                   |
|:----------------------------------------------|:------------------------------------------------------------------------------------------|
| TRPC_STREAM_SERVER_NETWORK_ERR = 201          | Server-side streaming network error                                                       |
| TRPC_STREAM_SERVER_MSG_EXCEED_LIMIT_ERR = 211 | Server-side streaming transmission error. For example, the streaming message is too long. |
| TRPC_STREAM_SERVER_ENCODE_ERR = 221           | Server-side streaming encoding error                                                      |
| TRPC_STREAM_SERVER_DECODE_ERR = 222           | Server-side streaming decoding error                                                      |
| TRPC_STREAM_SERVER_WRITE_END = 231            | Server-side streaming write stream end                                                    |
| TRPC_STREAM_SERVER_WRITE_OVERFLOW_ERR = 232   | Server-side streaming write overflow error                                                |
| TRPC_STREAM_SERVER_WRITE_CLOSE_ERR = 233      | Server-side streaming write close error                                                   |
| TRPC_STREAM_SERVER_WRITE_TIMEOUT_ERR = 234    | Server-side streaming write timeout error                                                 |
| TRPC_STREAM_SERVER_READ_END = 251             | Server-side streaming read stream end                                                     |
| TRPC_STREAM_SERVER_READ_CLOSE_ERR = 252       | Server-side streaming read close error                                                    |
| TRPC_STREAM_SERVER_READ_EMPTY_ERR = 253       | Server-side streaming read empty error                                                    |
| TRPC_STREAM_SERVER_READ_TIMEOUT_ERR = 254     | Server-side streaming read timeout error                                                  |

## Client-Side Streaming Error Codes

| Error Code                                    | Meaning                                                                                   |
|:----------------------------------------------|:------------------------------------------------------------------------------------------|
| TRPC_STREAM_CLIENT_NETWORK_ERR = 301          | Client-side streaming network error                                                       |
| TRPC_STREAM_CLIENT_MSG_EXCEED_LIMIT_ERR = 311 | Client-side streaming transmission error. For example, the streaming message is too long. |
| TRPC_STREAM_CLIENT_ENCODE_ERR = 321           | Client-side streaming encoding error                                                      |
| TRPC_STREAM_CLIENT_DECODE_ERR = 322           | Client-side streaming decoding error                                                      |
| TRPC_STREAM_CLIENT_WRITE_END = 331            | Client-side streaming write stream end                                                    |
| TRPC_STREAM_CLIENT_WRITE_OVERFLOW_ERR = 332   | Client-side streaming write overflow error                                                |
| TRPC_STREAM_CLIENT_WRITE_CLOSE_ERR = 333      | Client-side streaming write close error                                                   |
| TRPC_STREAM_CLIENT_WRITE_TIMEOUT_ERR = 334    | Client-side streaming write timeout error                                                 |
| TRPC_STREAM_CLIENT_READ_END = 351             | Client-side streaming read stream end                                                     |
| TRPC_STREAM_CLIENT_READ_CLOSE_ERR = 352       | Client-side streaming read close error                                                    |
| TRPC_STREAM_CLIENT_READ_EMPTY_ERR = 353       | Client-side streaming read empty error                                                    |
| TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR = 354     | Client-side streaming read timeout error                                                  |

## Undefined Errors

TRPC_STREAM_UNKNOWN_ERR = 1000