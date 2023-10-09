licenses(["notice"])

package(default_visibility = ["//visibility:public"])

load("@trpc_cpp//trpc:trpc.bzl", "trpc_proto_library")
trpc_proto_library(
    name = "stream_proto",
    srcs = ["stream.proto"],
    use_trpc_plugin = True,
    generate_mock_code = True,
)

cc_binary(
    name = "trpc_stream_server",
    srcs = ["stream_server.cc"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":stream_service",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_library(
    name = "stream_service",
    srcs = ["stream_service.cc"],
    hdrs = ["stream_service.h"],
    deps = [
        ":stream_proto",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/util/buffer:noncontiguous_buffer",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)