# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "object_pool_ptr",
    hdrs = ["object_pool_ptr.h"],
    deps = [
        ":object_pool",
    ],
)

cc_library(
    name = "object_pool",
    hdrs = ["object_pool.h"],
    deps = [
        ":disabled",
        ":global",
        ":shared_nothing",
        "//trpc/util/internal:never_destroyed",
    ],
)

cc_library(
    name = "disabled",
    hdrs = ["disabled.h"],
    deps = [
    ],
)

cc_library(
    name = "shared_nothing",
    srcs = ["shared_nothing.cc"],
    hdrs = ["shared_nothing.h"],
    deps = [
        ":util",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "object_pool_ptr_test",
    srcs = ["object_pool_ptr_test.cc"],
    deps = [
        ":object_pool_ptr",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "disabled_test",
    srcs = ["disabled_test.cc"],
    deps = [
        ":disabled",
        ":object_pool",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "shared_nothing_test",
    srcs = ["shared_nothing_test.cc"],
    deps = [
        ":object_pool",
        ":shared_nothing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "global",
    hdrs = ["global.h"],
    deps = [
        ":util",
        "//trpc/util:likely",
    ],
)

cc_test(
    name = "global_test",
    srcs = ["global_test.cc"],
    deps = [
        ":global",
        ":object_pool_ptr",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "util",
    hdrs = ["util.h"],
)
