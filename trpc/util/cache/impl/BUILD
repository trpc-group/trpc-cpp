licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "basic_cache",
    hdrs = ["basic_cache.h"],
    deps = [
        "//trpc/util/cache",
        "//trpc/util/concurrency:lightly_concurrent_hashmap",
    ],
)

cc_test(
    name = "basic_cache_test",
    srcs = ["basic_cache_test.cc"],
    deps = [
        ":basic_cache",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
