licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "client",
    srcs = ["client.cc"],
    defines = [] + select({
        "@trpc_cpp//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "@trpc_cpp//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [
        "//examples/features/prometheus/proxy:forward_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@com_github_gflags_gflags//:gflags",
    ],
)
