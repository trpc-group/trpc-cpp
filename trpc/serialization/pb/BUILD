licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "pb_serialization",
    srcs = ["pb_serialization.cc"],
    hdrs = ["pb_serialization.h"],
    deps = [
        "//trpc/serialization",
        "//trpc/util:likely",
        "//trpc/util/buffer:zero_copy_stream",
        "//trpc/util/log:logging",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "pb_serialization_test",
    srcs = ["pb_serialization_test.cc"],
    deps = [
        ":pb_serialization",
        "//trpc/serialization/testing:test_serialization_cc_proto",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
