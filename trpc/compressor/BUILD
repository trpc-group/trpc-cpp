licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "compressor_type",
    hdrs = ["compressor_type.h"],
    deps = [
        "//trpc/codec/trpc",
    ],
)

cc_library(
    name = "compressor",
    srcs = ["compressor.cc"],
    hdrs = ["compressor.h"],
    deps = [
        ":compressor_type",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_library(
    name = "compressor_factory",
    srcs = ["compressor_factory.cc"],
    hdrs = ["compressor_factory.h"],
    deps = [
        ":compressor",
        ":compressor_type",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "trpc_compressor",
    srcs = ["trpc_compressor.cc"],
    hdrs = ["trpc_compressor.h"],
    deps = [
        ":compressor_factory",
        ":compressor_type",
        "//trpc/compressor/gzip:gzip_compressor",
        "//trpc/compressor/lz4:lz4_compressor",
        "//trpc/compressor/snappy:snappy_compressor",
        "//trpc/compressor/zlib:zlib_compressor",
        "//trpc/log:trpc_log",
        "//trpc/util:likely",
        "//trpc/util/buffer",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "compressor_factory_test",
    srcs = ["compressor_factory_test.cc"],
    deps = [
        ":compressor",
        ":compressor_factory",
        "//trpc/compressor/testing:compressor_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trpc_compressor_test",
    srcs = ["trpc_compressor_test.cc"],
    deps = [
        ":compressor_factory",
        ":trpc_compressor",
        "//trpc/compressor/testing:compressor_testing",
        "//trpc/util/buffer",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
