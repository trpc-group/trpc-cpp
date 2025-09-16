# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fifo_cache",
    hdrs = ["fifo_cache.h"],
    deps = [
        "//trpc/util/cache",
    ],
)

cc_test(
    name = "fifo_cache_test",
    srcs = ["fifo_cache_test.cc"],
    deps = [
        ":fifo_cache",
        "//trpc/util/cache/impl:basic_cache",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "lru_cache",
    hdrs = ["lru_cache.h"],
    deps = [
        "//trpc/util/cache",
    ],
)

cc_test(
    name = "lru_cache_test",
    srcs = ["lru_cache_test.cc"],
    deps = [
        ":lru_cache",
        "//trpc/util/cache/impl:basic_cache",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "scheduled_cache",
    hdrs = ["scheduled_cache.h"],
    deps = [
        "//trpc/util/cache",
    ],
)

cc_test(
    name = "scheduled_cache_test",
    srcs = ["scheduled_cache_test.cc"],
    deps = [
        ":scheduled_cache",
        "//trpc/util/cache/impl:basic_cache",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)