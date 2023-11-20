licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files([
    "trpc_log_test.yaml",
    "default_log_test.yaml",
])

cc_library(
    name = "default_log",
    srcs = ["default_log.cc"],
    hdrs = ["default_log.h"],
    deps = [
        "//trpc/common/config:default_log_conf",
        "//trpc/common/config:default_value",
        "//trpc/common/config:local_file_sink_conf",
        "//trpc/common/config:stdout_sink_conf",
        "//trpc/log:logging",
        "//trpc/util/log",
        "//trpc/util/log/default/sinks/local_file:local_file_sink",
        "//trpc/util/log/default/sinks/stdout:stdout_sink",
        "@com_github_gabime_spdlog//:spdlog",
    ],
)

cc_test(
    name = "default_log_test",
    srcs = ["default_log_test.cc"],
    data = ["//trpc/util/log/default/testing:default_log_test.yaml"],
    deps = [
        ":default_log",
        "//trpc/common/config:config_helper",
        "//trpc/util/log:logging",
        "//trpc/util/log/default/testing:mock_sink",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
