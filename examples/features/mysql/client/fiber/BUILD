cc_binary(
    name = "fiber_client",
    srcs = ["fiber_client.cc"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "//trpc/client:client_context",
        "//trpc/client:make_client_context",
        "//trpc/client:trpc_client",
        "//trpc/client/mysql:mysql_service_proxy",
        "//trpc/common:runtime_manager",
        "//trpc/common:trpc_plugin",
        "//trpc/coroutine:fiber",
        "//trpc/util:latch",
        "//trpc/coroutine:future"
    ],
)
