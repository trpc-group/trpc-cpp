licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "chrono",
    srcs = ["chrono.cc"],
    hdrs = ["chrono.h"],
    copts = [
        "-lrt",
        "-lpthread",
    ],
    deps = [
        "//trpc/util:align",
    ],
)

cc_test(
    name = "chrono_test",
    srcs = ["chrono_test.cc"],
    deps = [
        ":chrono",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "time",
    srcs = ["time.cc"],
    hdrs = ["time.h"],
    deps = [
        ":chrono",
    ],
)

cc_test(
    name = "time_test",
    srcs = ["time_test.cc"],
    deps = [
        ":time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "tsc",
    srcs = ["tsc.cc"],
    hdrs = ["tsc.h"],
    deps = [
        ":chrono",
        "//trpc/util:likely",
    ],
)
