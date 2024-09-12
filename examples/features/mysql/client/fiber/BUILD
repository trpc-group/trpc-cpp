cc_binary(
    name = "fiber_client",
    srcs = ["fiber_client.cc"],
    linkopts = ["-ldl"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@trpc_cpp//trpc/client:client_context",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/client/mysql:mysql_service_proxy",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/coroutine:fiber",
        "@trpc_cpp//trpc/util:latch",
    ],
)
