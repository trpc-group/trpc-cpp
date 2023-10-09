licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "json_serialization",
    srcs = ["json_serialization.cc"],
    hdrs = ["json_serialization.h"],
    deps = [
        "//trpc/serialization",
        "//trpc/util:pb2json",
        "//trpc/util/log:logging",
        "@com_github_tencent_rapidjson//:rapidjson",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "json_serialization_test",
    srcs = ["json_serialization_test.cc"],
    deps = [
        ":json_serialization",
        "//trpc/serialization/testing:test_serialization_cc_proto",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
