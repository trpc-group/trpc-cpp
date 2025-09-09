licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "client_session",
    srcs = ["client_session.cc"],
    hdrs = ["client_session.h"],
    deps = [
        ":http2",
        ":request",
        ":response",
        ":session",
        "//trpc/util:deferred",
        "//trpc/util/log:logging",
        "@nghttp2",
    ],
)

cc_library(
    name = "http2",
    srcs = ["http2.cc"],
    hdrs = ["http2.h"],
    deps = [
        "//trpc/util/buffer",
        "//trpc/util/http:header",
        "//trpc/util/http:url",
        "//trpc/util/http:util",
        "@nghttp2",
    ],
)

cc_test(
    name = "http2_test",
    srcs = ["http2_test.cc"],
    deps = [
        ":http2",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "request",
    srcs = ["request.cc"],
    hdrs = ["request.h"],
    deps = [
        ":http2",
        "//trpc/util:function",
        "//trpc/util/http:request",
    ],
)

cc_test(
    name = "request_test",
    srcs = ["request_test.cc"],
    deps = [
        ":request",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "response",
    srcs = ["response.cc"],
    hdrs = ["response.h"],
    deps = [
        ":http2",
        "//trpc/util/http:response_hdrs",
    ],
)

cc_test(
    name = "response_test",
    srcs = ["response_test.cc"],
    deps = [
        ":response",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_session",
    srcs = ["server_session.cc"],
    hdrs = ["server_session.h"],
    deps = [
        ":http2",
        ":request",
        ":response",
        ":session",
        "//trpc/log:trpc_log",
        "//trpc/util:deferred",
        "//trpc/util:time",
        "//trpc/util/log:logging",
        "@nghttp2",
    ],
)

cc_library(
    name = "session",
    srcs = ["session.cc"],
    hdrs = ["session.h"],
    deps = [
        ":http2",
        "//trpc/transport/common:transport_message",
        "//trpc/util:function",
        "//trpc/util/buffer",
        "@nghttp2",
    ],
)

cc_test(
    name = "session_test",
    srcs = ["session_test.cc"],
    deps = [
        ":client_session",
        ":server_session",
        ":session",
        "//trpc/util:time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
