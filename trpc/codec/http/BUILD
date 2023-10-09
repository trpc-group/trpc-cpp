licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "http_client_codec",
    srcs = ["http_client_codec.cc"],
    hdrs = ["http_client_codec.h"],
    deps = [
        ":http_client_proto_checker_impl",
        ":http_protocol",
        "//trpc/client:client_context",
        "//trpc/codec:client_codec",
        "//trpc/common:status",
        "//trpc/serialization:serialization_factory",
        "//trpc/util:ref_ptr",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
    ],
)

cc_test(
    name = "http_client_codec_test",
    srcs = ["http_client_codec_test.cc"],
    deps = [
        ":http_client_codec",
        "//trpc/compressor/testing:compressor_testing",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/serialization:trpc_serialization",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_client_proto_checker_impl",
    srcs = ["http_client_proto_checker_impl.cc"],
    hdrs = ["http_proto_checker.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "//trpc/util/http/stream:http_client_stream_response",
        "@com_github_h2o_picohttpparser//:picohttpparser",
    ],
)

cc_test(
    name = "http_client_proto_checker_impl_test",
    srcs = ["http_client_proto_checker_impl_test.cc"],
    deps = [
        ":http_client_proto_checker_impl",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
        "//trpc/stream/http:http_client_stream",
        "//trpc/util/http/stream:http_client_stream_response",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_protocol",
    srcs = ["http_protocol.cc"],
    hdrs = ["http_protocol.h"],
    deps = [
        "//trpc/codec:protocol",
        "//trpc/codec/trpc",
        "//trpc/stream/http:http_stream",
        "//trpc/util:ref_ptr",
        "//trpc/util:string_helper",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
    ],
)

cc_test(
    name = "http_protocol_test",
    srcs = ["http_protocol_test.cc"],
    deps = [
        ":http_protocol",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_server_codec",
    srcs = ["http_server_codec.cc"],
    hdrs = ["http_server_codec.h"],
    deps = [
        ":http_protocol",
        ":http_server_proto_checker_impl",
        "//trpc/common:status",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/server:server_context",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
    ],
)

cc_test(
    name = "http_server_codec_test",
    srcs = ["http_server_codec_test.cc"],
    deps = [
        ":http_server_codec",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_stream_frame",
    hdrs = ["http_stream_frame.h"],
    deps = [
        "//trpc/util:ref_ptr",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/http:common",
        "//trpc/util/http:http_header",
        "//trpc/util/http/stream:http_start_line",
    ],
)

cc_library(
    name = "http_stream_parser",
    srcs = ["http_stream_parser.cc"],
    hdrs = ["http_stream_parser.h"],
    deps = [
        ":http_stream_frame",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/buffer:noncontiguous_buffer_view",
        "//trpc/util/http:common",
        "//trpc/util/http:util",
        "//trpc/util/log:logging",
        "@com_github_h2o_picohttpparser//:picohttpparser",
    ],
)

cc_library(
    name = "http_server_proto_checker_impl",
    srcs = ["http_server_proto_checker_impl.cc"],
    hdrs = ["http_proto_checker.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/stream/http:http_stream",
        "//trpc/transport/server/fiber:fiber_server_transport",
        "//trpc/util/http:request",
        "//trpc/util/log:logging",
        "@com_github_h2o_picohttpparser//:picohttpparser",
    ],
)

cc_test(
    name = "http_server_proto_checker_impl_test",
    srcs = ["http_server_proto_checker_impl_test.cc"],
    deps = [
        ":http_server_proto_checker_impl",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
