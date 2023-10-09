licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "auth",
    srcs = [],
    hdrs = ["auth.h"],
    deps = [
        "//trpc/common:plugin",
    ],
)

cc_library(
    name = "auth_factory",
    srcs = ["auth_factory.cc"],
    hdrs = ["auth_factory.h"],
    deps = [
        ":auth",
    ],
)

cc_test(
    name = "auth_factory_test",
    srcs = ["auth_factory_test.cc"],
    deps = [
        ":auth",
        ":auth_factory",
        "//trpc/auth/testing:auth_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "auth_center_follower_factory",
    srcs = ["auth_center_follower_factory.cc"],
    hdrs = ["auth_center_follower_factory.h"],
    deps = [
        ":auth",
    ],
)

cc_test(
    name = "auth_center_follower_factory_test",
    srcs = ["auth_center_follower_factory_test.cc"],
    deps = [
        ":auth",
        ":auth_center_follower_factory",
        "//trpc/auth/testing:auth_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
