# 集成测试（end2end testing）
## unary模块
### trpc子模块
目录组织如下所示：
```text
test/end2end/unary/trpc
├── BUILD
├── common_funcs.h
├── common_test.h
├── conf                  <- Test different environments using the same code by loading different configurations.
│   ├── BUILD
│   ├── trpc_client_fiber.yaml
│   ├── trpc_client_merge.yaml
│   ├── trpc_client_separate.yaml
│   ├── trpc_route_fiber.yaml
│   ├── trpc_route_merge.yaml
│   ├── trpc_route_separate.yaml
│   ├── trpc_server_fiber.yaml
│   ├── trpc_server_merge.yaml
│   └── trpc_server_separate.yaml
├── README.md
├── trpc_route_server.cc
├── trpc_route_server.h   <- trpc unary route server
├── trpc_server.cc
├── trpc_server.h         <- trpc unary server
├── trpc_server_transport_test_inc.h
├── trpc_test.cc          <- trpc unary test
├── trpc_test.fbs
└── trpc_test.proto
```
