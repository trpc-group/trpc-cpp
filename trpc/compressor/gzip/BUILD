licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "gzip_compressor",
    hdrs = ["gzip_compressor.h"],
    deps = [
        "//trpc/compressor",
        "//trpc/compressor:compressor_type",
        "//trpc/compressor/common:zlib_util",
    ],
)

cc_test(
    name = "gzip_compressor_test",
    srcs = ["gzip_compressor_test.cc"],
    deps = [
        ":gzip_compressor",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
