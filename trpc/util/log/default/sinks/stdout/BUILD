licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "stdout_sink",
    hdrs = ["stdout_sink.h"],
    deps = [
        "//trpc/common/config:default_log_conf",
        "//trpc/common/config:stdout_sink_conf",
        "//trpc/util:ref_ptr",
        "@com_github_gabime_spdlog//:spdlog",
    ],
)

cc_test(
    name = "stdout_sink_test",
    srcs = ["stdout_sink_test.cc"],
    deps = [
        ":stdout_sink",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
