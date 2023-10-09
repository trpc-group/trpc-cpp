licenses(["notice"])

package(default_visibility = ["//visibility:public"])

load("//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "stream_proto",
    srcs = ["stream.proto"],
    enable_explicit_link_proto = True,
    generate_new_mock_code = False,
    use_trpc_plugin = False,
)

cc_library(
    name = "mock_connection",
    srcs = [],
    hdrs = ["mock_connection.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "mock_stream_handler",
    srcs = [],
    hdrs = ["mock_stream_handler.h"],
    deps = [
        "//trpc/stream:stream_handler",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "mock_stream_provider",
    srcs = [],
    hdrs = ["mock_stream_provider.h"],
    deps = [
        "//trpc/stream:stream_provider",
        "@com_google_googletest//:gtest",
    ],
)
