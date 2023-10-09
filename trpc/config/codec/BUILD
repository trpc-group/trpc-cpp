# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "json_codec",
    hdrs = ["json_codec.h"],
    deps = [
        "//trpc/config/codec/common:common_codec",
        "//trpc/util/log:logging",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_library(
    name = "yaml_codec",
    hdrs = ["yaml_codec.h"],
    deps = [
        "//trpc/config/codec/common:common_codec",
        "//trpc/util/log:logging",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "toml_codec",
    hdrs = ["toml_codec.h"],
    deps = [
        "//trpc/config/codec/common:common_codec",
        "//trpc/util/log:logging",
        "@com_github_toruniina_toml11//:toml",
    ],
)
