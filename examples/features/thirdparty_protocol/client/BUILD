cc_binary(
    name = "client",
    srcs = ["client.cc"],
    deps = [
        "//examples/features/thirdparty_protocol/common:demo_client_codec",
        "//examples/features/thirdparty_protocol/common:demo_protocol",
        "@com_github_gflags_gflags//:gflags",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:non_rpc_service_proxy",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:trpc_plugin",
    ],
)
