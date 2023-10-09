package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "demo_server",
    srcs = ["demo_server.cc"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/log:trpc_log",
        "@trpc_cpp//trpc/server:rpc_method_handler",
        "@trpc_cpp//trpc/server:rpc_service_impl",
        "@trpc_cpp//trpc/server:rpc_service_method",
    ],
)