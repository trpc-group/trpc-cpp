licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "stream",
    srcs = ["stream.cc"],
    hdrs = ["stream.h"],
    deps = [
        "//trpc/stream/http/async:stream",
        "//trpc/util/http:request",
    ],
)

cc_test(
    name = "stream_test",
    srcs = ["stream_test.cc"],
    data = ["//trpc/runtime/threadmodel/testing:merge.yaml"],
    deps = [
        ":stream",
        "//trpc/common/config:trpc_config",
        "//trpc/future:future_utility",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/stream/http/async/server/testing:mock_server_async_stream",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_handler",
    srcs = ["stream_handler.cc"],
    hdrs = ["stream_handler.h"],
    deps = [
        "//trpc/filter:client_filter_controller",
        "//trpc/stream/http/async/server:stream",
        "//trpc/stream/http/common/server:stream_handler",
    ],
)

cc_test(
    name = "stream_handler_test",
    srcs = ["stream_handler_test.cc"],
    deps = [
        ":stream_handler",
        "//trpc/codec:codec_manager",
        "//trpc/codec/http:http_server_codec",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_connection_handler",
    srcs = ["stream_connection_handler.cc"],
    hdrs = ["stream_connection_handler.h"],
    deps = [
        "//trpc/codec:server_codec",
        "//trpc/codec:server_codec_factory",
        "//trpc/stream:server_stream_handler_factory",
        "//trpc/stream:stream_handler",
        "//trpc/stream:util",
        "//trpc/transport/server/default:server_connection_handler",
    ],
)

cc_test(
    name = "stream_connection_handler_test",
    srcs = ["stream_connection_handler_test.cc"],
    deps = [
        ":stream_connection_handler",
        "//trpc/codec:codec_manager",
        "//trpc/stream",
        "//trpc/stream/testing:mock_connection",
        "//trpc/stream/testing:mock_stream_handler",
        "//trpc/transport/server/default:default_server_transport",
        "@com_google_googletest//:gtest_main",
    ],
)
