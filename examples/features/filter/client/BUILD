package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "demo_client",
    srcs = ["demo_client.cc"],
    deps = [
        "//examples/features/filter/common:invoke_stat_filter",
        "//examples/features/filter/common:user_rpc_filter",
        "//examples/helloworld:helloworld_proto",
        "@com_github_gflags_gflags//:gflags",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/future:future_utility",
    ],
)
