licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "zlib_compressor",
    hdrs = ["zlib_compressor.h"],
    deps = [
        "//trpc/compressor",
        "//trpc/compressor:compressor_type",
        "//trpc/compressor/common:zlib_util",
    ],
)

cc_test(
    name = "zlib_compressor_test",
    srcs = ["zlib_compressor_test.cc"],
    deps = [
        ":zlib_compressor",
        "//trpc/compressor/testing:compressor_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@zlib",
    ],
)
