licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files(["separate.yaml"])

exports_files(["fiber.yaml"])

exports_files(["merge.yaml"])

cc_library(
    name = "thread_model_testing",
    hdrs = ["thread_model_testing.h"],
    deps = [
        "//trpc/runtime/threadmodel:thread_model",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
