licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "thread_model_op",
    srcs = ["thread_model_op.h"],
    hdrs = ["thread_model_op.h"],
    data = [":fiber_transport_test.yaml"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/util:latch",
    ],
)

cc_library(
    name = "mock_server",
    hdrs = ["mock_server.h"],
    deps = [
        "//trpc/coroutine:fiber",
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/transport/server/fiber:fiber_server_transport",
        "//trpc/util:net_util",
    ],
)

cc_library(
    name = "fake_server",
    hdrs = ["fake_server.h"],
    deps = [
        "//trpc/coroutine:fiber",
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/transport/server/fiber:fiber_server_transport",
        "//trpc/util:net_util",
    ],
)
