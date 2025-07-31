licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "noop_serialization",
    srcs = ["noop_serialization.cc"],
    hdrs = ["noop_serialization.h"],
    deps = [
        "//trpc/serialization",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "noop_serialization_test",
    srcs = ["noop_serialization_test.cc"],
    deps = [
        ":noop_serialization",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
