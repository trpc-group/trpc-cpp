load("@rules_cc//cc:defs.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_test(
    name = "http_sse_codec_test",
    srcs = ["http_sse_codec_test.cc"],
    size = "small",
    deps = [
        "//trpc/codec/http_sse:http_sse_codec",
        "//trpc/util/http/sse:http_sse",
        "//trpc/util/http/sse:http_sse_parser",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "http_sse_proto_checker_test",
    srcs = ["http_sse_proto_checker_test.cc"],
    size = "small",
    deps = [
        "//trpc/codec/http_sse:http_sse_codec",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "@com_google_googletest//:gtest_main",
    ],
)