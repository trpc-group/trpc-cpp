licenses(["notice"])

package(default_visibility = ["//visibility:public"])

load("@trpc_cpp//trpc:trpc.bzl", "trpc_proto_library")
trpc_proto_library(
    name = "stream_forward_proto",
    srcs = ["stream_forward.proto"],
    use_trpc_plugin = True,
    deps = [
        "//examples/features/trpc_stream/server:stream_proto",
    ],
)

cc_binary(
    name = "trpc_stream_forward_server",
    srcs = ["stream_forward_server.cc"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":stream_forward_service",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_library(
    name = "stream_forward_service",
    srcs = ["stream_forward_service.cc"],
    hdrs = ["stream_forward_service.h"],
    deps = [
        ":stream_forward_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/util/buffer:noncontiguous_buffer",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)