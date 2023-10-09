licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "snappy_compressor",
    srcs = ["snappy_compressor.cc"],
    hdrs = ["snappy_compressor.h"],
    deps = [
        "//trpc/compressor",
        "//trpc/compressor/common:util",
        "@com_github_google_snappy//:snappy",
    ],
)

cc_test(
    name = "snappy_compressor_test",
    srcs = ["snappy_compressor_test.cc"],
    deps = [
        ":snappy_compressor",
        "//trpc/compressor/testing:compressor_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
