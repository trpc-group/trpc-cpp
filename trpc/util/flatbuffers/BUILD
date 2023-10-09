package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fbs_interface",
    srcs = [
        "message_fbs.h",
    ],
)

cc_library(
    name = "trpc_fbs",
    srcs = ["trpc_fbs.cc"],
    hdrs = ["trpc_fbs.h"],
    deps = [
        ":fbs_interface",
        "//trpc/util/buffer",
        "//trpc/util/buffer:contiguous_buffer",
        "//trpc/util/buffer:noncontiguous_buffer",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_test(
    name = "trpc_fbs_test",
    srcs = ["trpc_fbs_test.cc"],
    deps = [
        ":trpc_fbs",
        "//trpc/tools/flatbuffers_tool/testing:greeter_fbs",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
