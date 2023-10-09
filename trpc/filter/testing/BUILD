licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "client_filter_testing",
    hdrs = ["client_filter_testing.h"],
    deps = [
        "//trpc/filter:client_filter_base",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_filter_testing",
    hdrs = ["server_filter_testing.h"],
    deps = [
        "//trpc/filter:server_filter_base",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
