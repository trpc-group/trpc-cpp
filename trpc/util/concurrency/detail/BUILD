# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lightly_concurrent_hashmap_impl",
    hdrs = ["lightly_concurrent_hashmap_impl.h"],
    deps = [
        "//trpc/util/hazptr",
    ],
)

cc_test(
    name = "lightly_concurrent_hashmap_impl_test",
    srcs = ["lightly_concurrent_hashmap_impl_test.cc"],
    deps = [
        ":lightly_concurrent_hashmap_impl",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
