cc_binary(
    name = "future_client",
    srcs = ["future_client.cc"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@trpc_cpp//trpc/client:client_context",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/client/mysql:mysql_service_proxy",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:trpc_plugin",
    ],
)
