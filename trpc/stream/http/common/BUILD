package(default_visibility = ["//visibility:public"])

cc_library(
    name = "stream",
    srcs = ["stream.cc"],
    hdrs = ["stream.h"],
    deps = [
        "//trpc/codec/http:http_stream_frame",
        "//trpc/stream:stream_handler",
        "//trpc/stream:stream_provider",
        "//trpc/stream/http:common",
        "//trpc/util:string_helper",
        "//trpc/util/http:common",
        "//trpc/util/http/sse:http_sse",
    ],
)

cc_test(
    name = "stream_test",
    srcs = ["stream_test.cc"],
    deps = [
        ":stream",
        "//trpc/stream/http/common/testing:mock_stream",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_handler",
    srcs = ["stream_handler.cc"],
    hdrs = ["stream_handler.h"],
    deps = [
        "//trpc/codec/http:http_stream_parser",
        "//trpc/stream:stream_handler",
        "//trpc/stream/http/common:stream",
    ],
)

cc_test(
    name = "stream_handler_test",
    srcs = ["stream_handler_test.cc"],
    deps = [
        ":stream_handler",
        "//trpc/stream/http/common/testing:mock_stream",
        "@com_google_googletest//:gtest_main",
    ],
)
