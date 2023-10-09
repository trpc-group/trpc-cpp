# End-to-end testing of HTTP/HTTPS.
## unary
### http
Directory tree of code like bellow:
```text
test/end2end/unary/http
├── conf <- Configuration for testings.
│   ├── http_rpc_test <- The same testing cases may be run with different configurations.
│   │   ├── http_client_fiber.yaml
│   │   ├── http_client_merge.yaml
│   │   ├── http_client_separate.yaml
│   │   ├── http_server_fiber.yaml
│   │   ├── http_server_merge.yaml
│   │   └── http_server_separate.yaml
│   ├── https_test
│   │   ├── <- It's same as `http_test`.
│   ├── https_test
│   │   ├── BUILD
│   │   ├── ca1 <- Certificate 1
│   │   ├── ca2 <- Certificate 2
│   │   ├── <- It's same as `http_test`.
├── http_rpc_test.cc <- End-to-end testing of RPC over HTTP.
├── http_rpc_server.h <- The server of RPC over HTTP.
├── https_test.cc <- End-to-end testing of HTTPS.
├── https_server.h <- HTTPS server.
└── http_test.cc <- End-to-testing of HTTP.
├── http_server.h <- HTTP server.
```

The main implementation includes the following test case designs.

| Test Category | Test Type | Test Case Design |
| --------- | ---- | ------------------------------------------- |
| General HTTP | Normal | Get/Post/Head/Options requests can respond normally (including synchronous and asynchronous interfaces) |
| General HTTP | Normal | HttpUnaryInvoke requests can respond normally (including synchronous and asynchronous interfaces) |
| General HTTP | Normal | Request header field filling and response header field parsing are normal |
| General HTTP | Boundary | Test for empty request and response |
| General HTTP | Boundary | Test for HTTP client requesting an incorrect URI |
| General HTTP | Boundary | GetJson receiving an incorrect response type |
| General HTTP | Boundary | Body is still present when the return code is not 2xx |
| General HTTP | Boundary | The client removes the header when receiving chunked response, but the server does not remove the header when receiving chunked request |
| General HTTP-RPC | Normal | Can access normally through PbInvoke/UnaryInvoke |
| General HTTP-RPC | Normal | Can access normally through RPC stub code |
| General HTTP-RPC | Normal | Can access normally through RPC stub code + pb alias |
| General HTTP-RPC | Boundary | Incorrect alias cannot be accessed normally |
| General HTTP-RPC | Boundary | Test for empty request and response |
| General HTTP-RPC | Boundary | Test for incorrect response type |
| General HTTP-RPC | Boundary | No body when the return code is not 2xx |
| HTTPS | Normal | HTTP-restful client can successfully access HTTP-restful server through HTTPS |
| HTTPS | Normal | HTTP-RPC client can successfully access HTTP-RPC server through HTTPS |
| HTTPS | Boundary | One of the server and client does not communicate through HTTPS (one of them is HTTP) |
| HTTPS | Boundary | Incorrect certificate configuration |