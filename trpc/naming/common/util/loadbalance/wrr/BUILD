# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "wrr_load_balance",
    srcs = ["wrr_load_balance.cc"],
    hdrs = ["wrr_load_balance.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "wrr_load_balance_test",
    srcs = ["wrr_load_balance_test.cc"],
    deps = [
        ":wrr_load_balance",
        "//trpc/naming:load_balance_factory",
        "//trpc/common:trpc_plugin",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)