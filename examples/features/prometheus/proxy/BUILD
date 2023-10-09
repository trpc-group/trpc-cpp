load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

trpc_proto_library(
    name = "forward_proto",
    srcs = ["forward.proto"],
    use_trpc_plugin = True,
    deps = [
        "//examples/helloworld:helloworld_proto",
    ],
)

cc_binary(
    name = "forward_server",
    srcs = ["forward_server.cc"],
    defines = [] + select({
        "@trpc_cpp//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "@trpc_cpp//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [
        ":forward_service",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

cc_library(
    name = "forward_service",
    srcs = ["forward_service.cc"],
    hdrs = ["forward_service.h"],
    defines = [] + select({
        "@trpc_cpp//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "@trpc_cpp//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [
        ":forward_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/log:trpc_log",
        "@trpc_cpp//trpc/metrics/prometheus:prometheus_metrics_api",
    ],
)
