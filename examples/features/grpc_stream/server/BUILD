licenses(["notice"])

package(default_visibility = ["//visibility:public"])

load("@trpc_cpp//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "stream_proto",
    srcs = ["stream.proto"],
    generate_new_mock_code = True,
    rootpath = "@trpc_cpp",
    use_trpc_plugin = True,
)

cc_binary(
    name = "grpc_stream_server",
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
        "//examples/features/grpc_stream/common:helper",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/util:time",
        "@trpc_cpp//trpc/util/buffer:noncontiguous_buffer",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)