licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "stream",
    srcs = ["stream.cc"],
    hdrs = ["stream.h"],
    deps = [
        "//trpc/common:async_timer",
        "//trpc/stream/http/common:stream",
    ],
)

cc_test(
    name = "stream_test",
    srcs = ["stream_test.cc"],
    data = ["//trpc/runtime/threadmodel/testing:merge.yaml"],
    deps = [
        ":stream",
        "//trpc/future:future_utility",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/stream/http/async/testing:mock_async_stream",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_reader_writer",
    srcs = ["stream_reader_writer.cc"],
    hdrs = ["stream_reader_writer.h"],
    deps = [
        "//trpc/stream/http/async/client:stream",
        "//trpc/stream/http/async/server:stream",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "//trpc/util/http:util",
    ],
)

cc_test(
    name = "stream_reader_writer_test",
    srcs = ["stream_reader_writer_test.cc"],
    data = ["//trpc/runtime/threadmodel/testing:merge.yaml"],
    deps = [
        ":stream_reader_writer",
        "//trpc/future:future_utility",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_function_handler",
    hdrs = ["stream_function_handler.h"],
    deps = [
        "//trpc/common/future",
        "//trpc/stream/http/async:stream_reader_writer",
        "//trpc/util/http:handler",
        "//trpc/util/log:logging",
    ],
)
