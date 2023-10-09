load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

trpc_proto_library(
    name = "trpc",
    srcs = ["trpc.proto"],
    native_proto_deps = [
        "//trpc/proto:trpc_proto",
    ],
)

cc_library(
    name = "trpc_proto_checker",
    srcs = ["trpc_proto_checker.cc"],
    hdrs = ["trpc_proto_checker.h"],
    deps = [
        ":trpc_protocol",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/util:likely",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "trpc_proto_checker_test",
    srcs = ["trpc_proto_checker_test.cc"],
    deps = [
        ":trpc_proto_checker",
        ":trpc_protocol",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
        "//trpc/runtime/iomodel/reactor/testing:mock_connection_testing",
        "//trpc/util/buffer:zero_copy_stream",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_protocol",
    srcs = ["trpc_protocol.cc"],
    hdrs = ["trpc_protocol.h"],
    deps = [
        ":trpc",
        "//trpc/codec:protocol",
        "//trpc/util:likely",
        "//trpc/util:ref_ptr",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/buffer:zero_copy_stream",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "trpc_protocol_test",
    srcs = ["trpc_protocol_test.cc"],
    deps = [
        ":trpc_protocol",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_server_codec",
    srcs = ["trpc_server_codec.cc"],
    hdrs = ["trpc_server_codec.h"],
    deps = [
        ":trpc_proto_checker",
        ":trpc_protocol",
        "//trpc/common:status",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/server:server_context",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_test(
    name = "trpc_server_codec_test",
    srcs = ["trpc_server_codec_test.cc"],
    deps = [
        ":trpc_protocol",
        ":trpc_server_codec",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_client_codec",
    srcs = ["trpc_client_codec.cc"],
    hdrs = ["trpc_client_codec.h"],
    deps = [
        ":trpc_proto_checker",
        ":trpc_protocol",
        "//trpc/client:client_context",
        "//trpc/codec:codec_helper",
        "//trpc/common:status",
        "//trpc/compressor:trpc_compressor",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/serialization:serialization_factory",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/buffer:zero_copy_stream",
        "//trpc/util/log:logging",
    ],
)
