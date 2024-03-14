# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "power_of_two",
    srcs = ["power_of_two.cc"],
    hdrs = ["power_of_two.h"],
    deps = [
    ],
)

cc_test(
    name = "power_of_two_test",
    srcs = ["power_of_two_test.cc"],
    deps = [
        ":power_of_two",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "random",
    hdrs = ["random.h"],
)

cc_library(
    name = "hash",
    srcs = ["hash.cc"],
    hdrs = ["hash.h"],
    deps = [
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "hash_test",
    srcs = ["hash_test.cc"],
    deps = [
        ":hash",
        "@com_google_googletest//:gtest_main",
    ],
)
