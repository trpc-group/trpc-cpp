licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "client",
    srcs = ["client.cc"],
    linkopts = ["-ldl"],
    deps = [
        "//examples/features/rpcz/proxy:rpcz_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/future:future_utility",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)
