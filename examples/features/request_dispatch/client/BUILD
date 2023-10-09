package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "fiber_client",
    srcs = ["fiber_client.cc"],
    linkopts = ["-ldl"],
    deps = [
        "//examples/helloworld:helloworld_proto",
        "@com_github_gflags_gflags//:gflags",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:runtime_manager",
    ],
)

cc_binary(
    name = "future_client",
    srcs = ["future_client.cc"],
    deps = [
        "//examples/helloworld:helloworld_proto",
        "@com_github_gflags_gflags//:gflags",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/future:future_utility",
        "@trpc_cpp//trpc/runtime/threadmodel/common:worker_thread",
        "@trpc_cpp//trpc/util/thread:latch",
    ],
)
