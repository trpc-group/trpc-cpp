licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
    deps = [
        "//trpc/util/buffer:zero_copy_stream",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "zlib_util",
    srcs = ["zlib_util.cc"],
    hdrs = ["zlib_util.h"],
    deps = [
        "//trpc/compressor:compressor_type",
        "//trpc/util/buffer:zero_copy_stream",
        "//trpc/util/log:logging",
        "@zlib",
    ],
)

cc_test(
    name = "zlib_util_test",
    srcs = ["zlib_util_test.cc"],
    deps = [
        ":zlib_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@zlib",
    ],
)
