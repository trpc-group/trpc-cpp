licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "grpc_client_stream_connection_handler",
    srcs = ["grpc_client_stream_connection_handler.cc"],
    hdrs = ["grpc_client_stream_connection_handler.h"],
    deps = [
        ":util",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec/grpc:grpc_protocol",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_connection",
        "//trpc/stream:client_stream_handler_factory",
        "//trpc/transport/client/fiber/common:fiber_client_connection_handler",
        "//trpc/transport/client/future/conn_complex:future_conn_complex_connection_handler",
        "//trpc/transport/client/future/conn_pool:future_conn_pool_connection_handler",
    ],
)

cc_test(
    name = "grpc_client_stream_connection_handler_test",
    srcs = ["grpc_client_stream_connection_handler_test.cc"],
    deps = [
        ":grpc_client_stream_connection_handler",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec:codec_manager",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/grpc:grpc_io_handler",
        "//trpc/stream/testing:mock_connection",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "grpc_client_stream_handler",
    srcs = ["grpc_client_stream_handler.cc"],
    hdrs = ["grpc_client_stream_handler.h"],
    deps = [
        "//trpc/client:client_context",
        "//trpc/codec/grpc:grpc_protocol",
        "//trpc/codec/grpc/http2:client_session",
        "//trpc/codec/grpc/http2:request",
        "//trpc/codec/grpc/http2:response",
        "//trpc/codec/grpc/http2:session",
        "//trpc/coroutine:fiber",
        "//trpc/stream:stream_handler",
    ],
)

cc_test(
    name = "grpc_client_stream_handler_test",
    srcs = ["grpc_client_stream_handler_test.cc"],
    deps = [
        ":grpc_client_stream_handler",
        "//trpc/codec:codec_manager",
        "//trpc/codec/grpc/http2/testing:mock_session",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "grpc_io_handler",
    srcs = ["grpc_io_handler.cc"],
    hdrs = ["grpc_io_handler.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/runtime/iomodel/reactor/common:default_io_handler",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "grpc_io_handler_test",
    srcs = ["grpc_io_handler_test.cc"],
    deps = [
        ":grpc_io_handler",
        "//trpc/stream/testing:mock_connection",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "grpc_server_stream",
    srcs = ["grpc_server_stream.cc"],
    hdrs = ["grpc_server_stream.h"],
    deps = [
        ":grpc_stream",
        "//trpc/codec/grpc/http2:server_session",
        "//trpc/filter:server_filter_controller",
        "//trpc/server:method",
        "//trpc/server:server_context",
        "//trpc/util:time",
    ],
)

cc_test(
    name = "grpc_server_stream_test",
    srcs = ["grpc_server_stream_test.cc"],
    deps = [
        ":grpc_server_stream",
        "//trpc/codec:codec_manager",
        "//trpc/codec/grpc/http2/testing:mock_session",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/server:rpc_service_impl",
        "//trpc/server:server_context",
        "//trpc/server:stream_rpc_method_handler",
        "//trpc/stream",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "grpc_server_stream_connection_handler",
    srcs = ["grpc_server_stream_connection_handler.cc"],
    hdrs = ["grpc_server_stream_connection_handler.h"],
    deps = [
        ":grpc_io_handler",
        ":util",
        "//trpc/codec:server_codec_factory",
        "//trpc/codec/grpc:grpc_protocol",
        "//trpc/codec/testing:server_codec_testing",
        "//trpc/coroutine:fiber",
        "//trpc/stream:server_stream_handler_factory",
        "//trpc/stream:util",
        "//trpc/transport/server/default:server_connection_handler",
        "//trpc/transport/server/fiber:fiber_server_connection_handler",
    ],
)

cc_test(
    name = "grpc_server_stream_connection_handler_test",
    srcs = ["grpc_server_stream_connection_handler_test.cc"],
    deps = [
        ":grpc_server_stream_connection_handler",
        "//trpc/codec:codec_manager",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/testing:mock_connection",
        "//trpc/stream/testing:mock_stream_handler",
        "//trpc/transport/server/default:default_server_transport",
        "//trpc/transport/server/fiber:fiber_server_transport",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "grpc_server_stream_handler",
    srcs = ["grpc_server_stream_handler.cc"],
    hdrs = ["grpc_server_stream_handler.h"],
    deps = [
        ":grpc_server_stream",
        "//trpc/codec/grpc/http2:server_session",
        "//trpc/coroutine:fiber",
        "//trpc/filter:server_filter_controller",
    ],
)

cc_test(
    name = "grpc_server_stream_handler_test",
    srcs = ["grpc_server_stream_handler_test.cc"],
    deps = [
        ":grpc_server_stream_handler",
        "//trpc/codec:codec_manager",
        "//trpc/codec/grpc/http2/testing:mock_session",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "grpc_stream",
    srcs = ["grpc_stream.cc"],
    hdrs = ["grpc_stream.h"],
    deps = [
        "//trpc/codec/grpc:grpc_protocol",
        "//trpc/codec/grpc:grpc_stream_frame",
        "//trpc/codec/grpc/http2:session",
        "//trpc/filter:filter_controller",
        "//trpc/stream:common_stream",
        "//trpc/stream:stream_var",
    ],
)

cc_test(
    name = "grpc_stream_test",
    srcs = ["grpc_stream_test.cc"],
    deps = [
        ":grpc_stream",
        "//trpc/codec/grpc/http2/testing:mock_session",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/common:io_handler",
        "//trpc/util:function",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "util_test",
    srcs = ["util_test.cc"],
    deps = [
        ":util",
        "//trpc/runtime/iomodel/reactor/common:default_io_handler",
        "//trpc/stream/testing:mock_connection",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
