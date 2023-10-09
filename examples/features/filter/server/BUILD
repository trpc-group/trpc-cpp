package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "demo_server",
    srcs = ["demo_server.cc"],
    linkopts = [
        "-ldl",
    ] + select({
        "@trpc_cpp//trpc:trpc_enable_profiler": ["/usr/lib64/libtcmalloc_and_profiler.so"],
        "//conditions:default": [],
    }),
    deps = [
        "//examples/helloworld:helloworld_proto",
        "//examples/features/filter/common:invoke_stat_filter",
        "//examples/features/filter/common:user_rpc_filter",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/common/logging:trpc_logging",
    ],
)
