package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "demo_server",
    srcs = ["demo_server.cc"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        "//examples/features/thirdparty_protocol/common:demo_protocol",
        "//examples/features/thirdparty_protocol/common:demo_server_codec",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/server:method",
        "@trpc_cpp//trpc/server:non_rpc_method_handler",
        "@trpc_cpp//trpc/server:non_rpc_service_impl",
        "@trpc_cpp//trpc/server:non_rpc_service_method",
    ],
)