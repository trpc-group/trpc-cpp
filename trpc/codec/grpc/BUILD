licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "grpc_client_codec",
    srcs = ["grpc_client_codec.cc"],
    hdrs = ["grpc_client_codec.h"],
    deps = [
        ":grpc_protocol",
        "//trpc/client:client_context",
        "//trpc/codec:client_codec",
        "//trpc/codec/grpc/http2:request",
        "//trpc/codec/grpc/http2:response",
        "//trpc/common:status",
        "//trpc/compressor:trpc_compressor",
        "//trpc/serialization:serialization_factory",
        "//trpc/stream:stream_handler",
        "//trpc/util/http:util",
    ],
)

cc_test(
    name = "grpc_client_codec_test",
    srcs = ["grpc_client_codec_test.cc"],
    deps = [
        ":grpc_client_codec",
        "//trpc/compressor:compressor_factory",
        "//trpc/compressor/testing:compressor_testing",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/serialization:serialization_factory",
        "//trpc/serialization:trpc_serialization",
        "//trpc/stream/testing:mock_connection",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "grpc_protocol",
    srcs = ["grpc_protocol.cc"],
    hdrs = ["grpc_protocol.h"],
    deps = [
        "//trpc/codec:protocol",
        "//trpc/codec/grpc/http2:request",
        "//trpc/codec/grpc/http2:response",
        "//trpc/common:trpc_version",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/http:util",
    ],
)

cc_test(
    name = "grpc_protocol_test",
    srcs = ["grpc_protocol_test.cc"],
    deps = [
        ":grpc_protocol",
        "//trpc/codec/trpc",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "grpc_server_codec",
    srcs = ["grpc_server_codec.cc"],
    hdrs = ["grpc_server_codec.h"],
    deps = [
        ":grpc_protocol",
        "//trpc/codec:server_codec",
        "//trpc/codec/grpc:grpc_stream_frame",
        "//trpc/codec/grpc/http2:request",
        "//trpc/codec/grpc/http2:response",
        "//trpc/common:status",
        "//trpc/compressor:trpc_compressor",
        "//trpc/serialization:serialization_factory",
        "//trpc/server:server_context",
        "//trpc/util/http:util",
    ],
)

cc_test(
    name = "grpc_server_codec_test",
    srcs = ["grpc_server_codec_test.cc"],
    deps = [
        ":grpc_server_codec",
        "//trpc/codec/trpc",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/stream/testing:mock_connection",
        "//trpc/stream/testing:mock_stream_handler",
        "//trpc/util/buffer:zero_copy_stream",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "grpc_stream_frame",
    srcs = [],
    hdrs = ["grpc_stream_frame.h"],
    deps = [
        "//trpc/codec/grpc:grpc_protocol",
        "//trpc/codec/grpc/http2:request",
        "//trpc/stream:stream_handler",
    ],
)

cc_test(
    name = "grpc_stream_frame_test",
    srcs = ["grpc_stream_frame_test.cc"],
    deps = [
        ":grpc_stream_frame",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
