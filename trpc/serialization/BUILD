licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "serialization_type",
    hdrs = ["serialization_type.h"],
    deps = [
        "//trpc/codec/trpc",
    ],
)

cc_library(
    name = "serialization",
    hdrs = ["serialization.h"],
    deps = [
        ":serialization_type",
        "//trpc/util/buffer",
    ],
)

cc_library(
    name = "serialization_factory",
    srcs = ["serialization_factory.cc"],
    hdrs = ["serialization_factory.h"],
    deps = [
        ":serialization",
        "//trpc/util:likely",
    ],
)

cc_library(
    name = "trpc_serialization",
    srcs = ["trpc_serialization.cc"],
    hdrs = ["trpc_serialization.h"],
    deps = [
        ":serialization_factory",
        "//trpc/serialization/flatbuffers:fbs_serialization",
        "//trpc/serialization/json:json_serialization",
        "//trpc/serialization/noop:noop_serialization",
        "//trpc/serialization/pb:pb_serialization",
    ],
)

cc_test(
    name = "serialization_factory_test",
    srcs = ["serialization_factory_test.cc"],
    deps = [
        ":serialization",
        ":serialization_factory",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
