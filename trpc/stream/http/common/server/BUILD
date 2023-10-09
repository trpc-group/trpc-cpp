package(default_visibility = ["//visibility:public"])

cc_library(
    name = "stream_handler",
    srcs = ["stream_handler.cc"],
    hdrs = ["stream_handler.h"],
    deps = [
        "//trpc/codec:server_codec_factory",
        "//trpc/stream/http/common:stream_handler",
    ],
)

cc_test(
    name = "stream_handler_test",
    srcs = ["stream_handler_test.cc"],
    deps = [
        ":stream_handler",
        "//trpc/codec:codec_manager",
        "@com_google_googletest//:gtest_main",
    ],
)
