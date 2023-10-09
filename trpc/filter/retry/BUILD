load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "retry_limit_client_filter",
    srcs = [
        "retry_limit_client_filter.cc",
    ],
    hdrs = ["retry_limit_client_filter.h"],
    deps = [
        "//trpc/client:client_context",
        "//trpc/common/config:retry_conf",
        "//trpc/filter:client_filter_base",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "retry_limit_client_filter_test",
    srcs = ["retry_limit_client_filter_test.cc"],
    deps = [
        ":retry_limit_client_filter",
        "@com_google_googletest//:gtest_main",
    ],
)
