# trpc/util/http/sse/test/BUILD
cc_test(
    name = "sse_event_test",
    srcs = ["sse_event_test.cc"],
    timeout = "short",
    deps = [
        "//trpc/util/http/sse:http_sse",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "sse_parser_test",
    srcs = ["sse_parser_test.cc"],
    timeout = "short",
    deps = [
        "//trpc/util/http/sse:http_sse_parser",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)