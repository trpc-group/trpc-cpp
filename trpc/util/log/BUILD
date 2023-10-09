licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "log",
    hdrs = ["log.h"],
    deps = [
        "//trpc/util:ref_ptr",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_library(
    name = "logging",
    srcs = ["logging.cc"],
    hdrs = ["logging.h"],
    deps = [
        ":printf_like",
        ":python_like",
        ":stream_like",
        "//trpc/common/config:config_helper",
        "//trpc/util/log/default:default_log",
    ],
)

cc_test(
    name = "logging_test",
    srcs = ["logging_test.cc"],
    data = ["//trpc/util/log/testing:trpc_log_test.yaml"],
    deps = [
        ":logging",
        "//trpc/util:time",
        "//trpc/util/log/default/testing:mock_sink",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_like",
    hdrs = [
        "stream_like.h",
    ],
    deps = [
        ":log",
        "//trpc/common/config:default_value",
        "//trpc/util/log/default:default_log",
    ],
)

cc_test(
    name = "stream_like_test",
    srcs = [
        "logging.cc",
        "stream_like_test.cc",
    ],
    data = ["//trpc/util/log/testing:trpc_log_test.yaml"],
    deps = [
        ":logging",
        ":stream_like",
        "//trpc/client:client_context",
        "//trpc/common/config:config_helper",
        "//trpc/server:server_context",
        "//trpc/util/log/default/testing:mock_sink",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "python_like",
    hdrs = [
        "python_like.h",
    ],
    deps = [
        ":log",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_test(
    name = "python_like_test",
    srcs = [
        "logging.cc",
        "python_like_test.cc",
    ],
    data = ["//trpc/util/log/testing:trpc_log_test.yaml"],
    deps = [
        ":logging",
        ":python_like",
        "//trpc/client:client_context",
        "//trpc/common/config:config_helper",
        "//trpc/server:server_context",
        "//trpc/util/log/default/testing:mock_sink",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "printf_like",
    hdrs = [
        "printf_like.h",
    ],
    deps = [
        ":log",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_test(
    name = "printf_like_test",
    srcs = ["printf_like_test.cc"],
    data = ["//trpc/util/log/testing:trpc_log_test.yaml"],
    deps = [
        ":logging",
        ":printf_like",
        "//trpc/client:client_context",
        "//trpc/common/config:config_helper",
        "//trpc/server:server_context",
        "//trpc/util/log/default/testing:mock_sink",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "log_test",
    srcs = ["log_test.cc"],
    deps = [
        "log",
        "//trpc/util/log/testing:mock_log",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
