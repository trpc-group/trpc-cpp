licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "stream",
    srcs = ["stream.cc"],
    hdrs = ["stream.h"],
    deps = [
        "//trpc/client:client_context",
        "//trpc/filter:client_filter_controller",
        "//trpc/stream/http/async:stream",
        "//trpc/util/http:response",
    ],
)

cc_test(
    name = "stream_test",
    srcs = ["stream_test.cc"],
    data = [
        "//trpc/runtime/threadmodel/testing:merge.yaml",
    ],
    deps = [
        ":stream",
        "//trpc/filter/testing:client_filter_testing",
        "//trpc/future:future_utility",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_connection_handler",
    srcs = ["stream_connection_handler.cc"],
    hdrs = ["stream_connection_handler.h"],
    deps = [
        "//trpc/stream:client_stream_handler_factory",
        "//trpc/transport/client/future/conn_pool:future_conn_pool_connection_handler",
    ],
)

cc_test(
    name = "stream_connection_handler_test",
    srcs = ["stream_connection_handler_test.cc"],
    deps = [
        ":stream_connection_handler",
        "//trpc/stream/testing:mock_connection",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_handler",
    srcs = ["stream_handler.cc"],
    hdrs = ["stream_handler.h"],
    deps = [
        "//trpc/stream/http/async/client:stream",
        "//trpc/stream/http/common/client:stream_handler",
    ],
)

cc_test(
    name = "stream_handler_test",
    srcs = ["stream_handler_test.cc"],
    deps = [
        ":stream_handler",
        "@com_google_googletest//:gtest_main",
    ],
)
