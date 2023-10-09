# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files([
    "test.yaml",
    "test_load.json",
    "test_load.yaml",
    "test_load.toml",
])

cc_library(
    name = "config_plugin_testing",
    hdrs = ["config_plugin_testing.h"],
    deps = [
        "//trpc/config:config_factory",
    ],
)

cc_library(
    name = "codec_plugin_testing",
    hdrs = ["codec_plugin_testing.h"],
    deps = [
        "//trpc/config:codec_factory",
    ],
)

cc_library(
    name = "provider_plugin_testing",
    hdrs = ["provider_plugin_testing.h"],
    deps = [
        "//trpc/config:provider_factory",
    ],
)
