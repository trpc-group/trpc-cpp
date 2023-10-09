licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "client",
    srcs = ["client.cc"],
    linkopts = ["-ldl"],
    deps = [
        "@com_github_tencent_rapidjson//:rapidjson",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/coroutine:fiber",
        "@trpc_cpp//trpc/util/log:logging",
        "@trpc_cpp//trpc/util/thread:latch",
    ],
)
