# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lz4_compressor",
    srcs = ["lz4_compressor.cc"],
    hdrs = ["lz4_compressor.h"],
    deps = [
        ":lz4_util",
        "//trpc/compressor",
        "//trpc/compressor:compressor_type",
    ],
)

cc_library(
    name = "lz4_util",
    srcs = ["lz4_util.cc"],
    hdrs = ["lz4_util.h"],
    deps = [
        "//trpc/compressor:compressor_type",
        "//trpc/compressor/common:util",
        "//trpc/util/buffer:zero_copy_stream",
        "//trpc/util/log:logging",
        "@com_github_lz4_lz4//:lz4",
    ],
)

cc_test(
    name = "lz4_compressor_test",
    srcs = ["lz4_compressor_test.cc"],
    deps = [
        ":lz4_compressor",
        "//trpc/compressor/testing:compressor_testing",
        "//trpc/util/buffer",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
