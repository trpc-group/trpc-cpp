cc_binary(
    name = "client",
    srcs = ["client.cc"],
    deps = [
        "//examples/features/future_forward/proxy:forward_proto",
        "@com_github_gflags_gflags//:gflags",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/future:future_utility",
        "@trpc_cpp//trpc/util:time",
    ],
)
