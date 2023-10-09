licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "trpc_client_stream",
    srcs = ["trpc_client_stream.cc"],
    hdrs = ["trpc_client_stream.h"],
    deps = [
        ":trpc_stream",
        "//trpc/client:client_context",
        "//trpc/filter:client_filter_controller",
        "//trpc/serialization:serialization_factory",
    ],
)

cc_test(
    name = "trpc_client_stream_test",
    srcs = ["trpc_client_stream_test.cc"],
    deps = [
        ":trpc_client_stream",
        "//trpc/client:client_context",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/serialization:serialization_factory",
        "//trpc/stream/testing:mock_stream_handler",
        "//trpc/stream/trpc/testing:trpc_stream_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_client_stream_connection_handler",
    srcs = ["trpc_client_stream_connection_handler.cc"],
    hdrs = ["trpc_client_stream_connection_handler.h"],
    deps = [
        "//trpc/codec:client_codec_factory",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/coroutine:fiber",
        "//trpc/stream:client_stream_handler_factory",
        "//trpc/transport/client/fiber/common:fiber_client_connection_handler",
        "//trpc/transport/client/future/conn_complex:future_conn_complex_connection_handler",
        "//trpc/transport/client/future/conn_pool:future_conn_pool_connection_handler",
    ],
)

cc_test(
    name = "trpc_client_stream_connection_handler_test",
    srcs = ["trpc_client_stream_connection_handler_test.cc"],
    deps = [
        ":trpc_client_stream_connection_handler",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec:codec_manager",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/testing:mock_connection",
        "//trpc/stream/testing:mock_stream_handler",
        "//trpc/stream/trpc/testing:trpc_stream_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_client_stream_handler",
    srcs = ["trpc_client_stream_handler.cc"],
    hdrs = ["trpc_client_stream_handler.h"],
    deps = [
        ":trpc_client_stream",
        ":trpc_stream_handler",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/filter:client_filter_controller",
        "//trpc/util:unique_id",
    ],
)

cc_test(
    name = "trpc_client_stream_handler_test",
    srcs = ["trpc_client_stream_handler_test.cc"],
    deps = [
        ":trpc_client_stream_handler",
        "//trpc/client:client_context",
        "//trpc/codec:codec_manager",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/trpc/testing:trpc_stream_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_server_stream",
    srcs = ["trpc_server_stream.cc"],
    hdrs = ["trpc_server_stream.h"],
    deps = [
        ":trpc_stream",
        "//trpc/filter:server_filter_controller",
        "//trpc/server:server_context",
    ],
)

cc_test(
    name = "trpc_server_stream_test",
    srcs = ["trpc_server_stream_test.cc"],
    deps = [
        ":trpc_server_stream",
        "//trpc/codec/trpc:trpc_server_codec",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/server:server_context",
        "//trpc/stream/testing:mock_stream_handler",
        "//trpc/stream/trpc/testing:trpc_stream_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_server_stream_connection_handler",
    srcs = ["trpc_server_stream_connection_handler.cc"],
    hdrs = ["trpc_server_stream_connection_handler.h"],
    deps = [
        "//trpc/codec:server_codec_factory",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/stream:server_stream_handler_factory",
        "//trpc/stream:util",
        "//trpc/transport/server/default:server_connection_handler",
        "//trpc/transport/server/fiber:fiber_server_connection_handler",
    ],
)

cc_test(
    name = "trpc_server_stream_connection_handler_test",
    srcs = ["trpc_server_stream_connection_handler_test.cc"],
    deps = [
        ":trpc_server_stream_connection_handler",
        "//trpc/codec:codec_manager",
        "//trpc/codec/testing:server_codec_testing",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/testing:mock_connection",
        "//trpc/stream/testing:mock_stream_handler",
        "//trpc/stream/trpc/testing:trpc_stream_testing",
        "//trpc/transport/server/default:default_server_transport",
        "//trpc/transport/server/fiber:fiber_server_transport",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_server_stream_handler",
    srcs = ["trpc_server_stream_handler.cc"],
    hdrs = ["trpc_server_stream_handler.h"],
    deps = [
        ":trpc_server_stream",
        ":trpc_stream_handler",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/filter:server_filter_controller",
    ],
)

cc_test(
    name = "trpc_server_stream_handler_test",
    srcs = ["trpc_server_stream_handler_test.cc"],
    deps = [
        ":trpc_server_stream_handler",
        "//trpc/codec:codec_manager",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/trpc/testing:trpc_stream_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_stream",
    srcs = ["trpc_stream.cc"],
    hdrs = ["trpc_stream.h"],
    deps = [
        ":trpc_stream_flow_controller",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/stream:common_stream",
    ],
)

cc_test(
    name = "trpc_stream_test",
    srcs = ["trpc_stream_test.cc"],
    deps = [
        ":trpc_stream",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_stream_flow_controller",
    hdrs = ["trpc_stream_flow_controller.h"],
    deps = [
        "//trpc/util:ref_ptr",
    ],
)

cc_test(
    name = "trpc_stream_flow_controller_test",
    srcs = ["trpc_stream_flow_controller_test.cc"],
    deps = [
        ":trpc_stream_flow_controller",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_stream_handler",
    srcs = ["trpc_stream_handler.cc"],
    hdrs = ["trpc_stream_handler.h"],
    deps = [
        ":trpc_stream",
        "//trpc/coroutine:fiber",
        "//trpc/stream:stream_handler",
        "//trpc/util:function",
    ],
)

cc_test(
    name = "trpc_stream_handler_test",
    srcs = ["trpc_stream_handler_test.cc"],
    deps = [
        ":trpc_stream_handler",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/trpc/testing:trpc_stream_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
