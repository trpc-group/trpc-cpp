# trpc/codec/http_sse/BUILD
cc_library(
    name = "http_sse_codec",
    srcs = [
        "http_sse_codec.cc",
    ],
    hdrs = [
        "http_sse_codec.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/codec/http:http_client_codec",
        "//trpc/codec/http:http_server_codec",
        "//trpc/codec/http:http_protocol",
        "//trpc/common:status",
        "//trpc/log:trpc_log",
        "//trpc/util/http/sse:http_sse",
    ],
)

cc_test(
    name = "http_sse_codec_test",
    srcs = ["test/http_sse_codec_test.cc"],
    deps = [
        ":http_sse_codec",
        "//trpc/codec/http:http_protocol",
        "//trpc/client:client_context",
        "//trpc/server:server_context",
        "//trpc/util/http/sse:http_sse",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)