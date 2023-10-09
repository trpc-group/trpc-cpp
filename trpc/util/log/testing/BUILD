licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files([
    "trpc_log_test.yaml",
])

cc_library(
    name = "mock_log",
    hdrs = [
        "mock_log.h",
    ],
    deps = [
        "//trpc/util/log",
        "//trpc/util/log/default:default_log",
    ],
)
