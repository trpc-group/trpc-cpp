# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "local_file_provider",
    hdrs = ["local_file_provider.h"],
    deps = [
        "//trpc/common/config:local_file_provider_conf",
        "//trpc/config:provider",
        "//trpc/config/default:loader",
    ],
)

cc_test(
    name = "local_file_provider_test",
    srcs = ["local_file_provider_test.cc"],
    data = [
        "//trpc/config/testing:test.yaml",
        "//trpc/config/testing:test_load.json",
        "//trpc/config/testing:test_load.toml",
        "//trpc/config/testing:test_load.yaml",
    ],
    deps = [
        ":local_file_provider",
        "//trpc/config:trpc_conf",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
