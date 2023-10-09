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
    name = "future_forward",
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":forward_server",
    ],
)

cc_library(
    name = "forward_server",
    srcs = ["forward_server.cc"],
    hdrs = ["forward_server.h"],
    deps = [
        ":forward_service",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

cc_library(
    name = "forward_service",
    srcs = ["forward_service.cc"],
    hdrs = ["forward_service.h"],
    deps = [
        ":forward_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/future:future_utility",
        "@trpc_cpp//trpc/log:trpc_log",
    ],
)
