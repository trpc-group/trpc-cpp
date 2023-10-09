package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "transparentserver",
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":transparent_server",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

cc_library(
    name = "transparent_server",
    srcs = ["transparent_server.cc"],
    hdrs = ["transparent_server.h"],
    deps = [
        ":transparent_service",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/log:trpc_log",
    ],
)

cc_library(
    name = "transparent_service",
    srcs = ["transparent_service.cc"],
    hdrs = ["transparent_service.h"],
    deps = [
        "@trpc_cpp//examples/helloworld:helloworld_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:rpc_service_proxy",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/log:trpc_log",
        "@trpc_cpp//trpc/server/forward:forward_rpc_method_handler",
        "@trpc_cpp//trpc/server/forward:forward_rpc_service_impl",
        "@trpc_cpp//trpc/util/buffer",
    ],
)
