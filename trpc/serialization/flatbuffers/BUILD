licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fbs_serialization",
    srcs = ["fbs_serialization.cc"],
    hdrs = ["fbs_serialization.h"],
    deps = [
        "//trpc/serialization",
        "//trpc/util:likely",
        "//trpc/util/buffer:zero_copy_stream",
        "//trpc/util/flatbuffers:fbs_interface",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "fbs_serialization_test",
    srcs = ["fbs_serialization_test.cc"],
    deps = [
        ":fbs_serialization",
        "//trpc/proto/testing:helloworld_fbs",
        "//trpc/util/flatbuffers:trpc_fbs",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
