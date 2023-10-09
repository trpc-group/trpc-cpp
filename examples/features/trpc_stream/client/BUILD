licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "client",
    srcs = ["client.cc"],
    linkopts = ["-ldl"],
    deps = [
        "//examples/features/trpc_stream/server:stream_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/coroutine:fiber",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_binary(
    name = "rawdata_stream_client",
    srcs = ["rawdata_stream_client.cc"],
    linkopts = ["-ldl"],
    deps = [
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:rpc_service_proxy",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/coroutine:fiber",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)