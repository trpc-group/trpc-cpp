cc_binary(
    name = "client",
    srcs = ["client.cc"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@com_github_tencent_rapidjson//:rapidjson",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:rpc_service_proxy",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/client:trpc_client",
    ],
)
