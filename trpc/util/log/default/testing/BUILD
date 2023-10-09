licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files([
    "default_log_test.yaml",
])

cc_library(
    name = "mock_sink",
    hdrs = ["mock_sink.h"],
    deps = [
        "//trpc/util:ref_ptr",
        "@com_github_gabime_spdlog//:spdlog",
    ],
)
