# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "provider",
    hdrs = ["provider.h"],
    deps = [
        "//trpc/common:plugin",
    ],
)

cc_library(
    name = "provider_factory",
    hdrs = ["provider_factory.h"],
    deps = [
        ":provider",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "provider_factory_test",
    srcs = ["provider_factory_test.cc"],
    deps = [
        "//trpc/config/testing:provider_plugin_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "codec",
    hdrs = ["codec.h"],
    deps = [
        "//trpc/common:plugin",
    ],
)

cc_library(
    name = "codec_factory",
    hdrs = ["codec_factory.h"],
    deps = [
        ":codec",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "codec_factory_test",
    srcs = ["codec_factory_test.cc"],
    deps = [
        "//trpc/config/testing:codec_plugin_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "config",
    hdrs = ["config.h"],
    deps = [
        "//trpc/common:plugin",
    ],
)

cc_library(
    name = "config_factory",
    hdrs = ["config_factory.h"],
    deps = [
        ":config",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "config_factory_test",
    srcs = ["config_factory_test.cc"],
    deps = [
        "//trpc/config/testing:config_plugin_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_conf",
    srcs = [
        "trpc_conf.cc",
        "trpc_conf_compatible.cc",
    ],
    hdrs = [
        "trpc_conf.h",
        "trpc_conf_compatible.h",
        "trpc_conf_deprecated.h",
    ],
    deps = [
        "//trpc/common/config:yaml_parser",
        "//trpc/config:config_factory",
        "//trpc/config/codec:json_codec",
        "//trpc/config/codec:toml_codec",
        "//trpc/config/codec:yaml_codec",
        "//trpc/config/default:default_config",
        "//trpc/config/default:loader",
        "//trpc/config/provider/local_file:local_file_provider",
        "//trpc/util:string_util",
        "//trpc/util/log:logging",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_github_open_source_parsers_jsoncpp//:jsoncpp",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "trpc_conf_test",
    srcs = ["trpc_conf_test.cc"],
    deps = [
        ":trpc_conf",
        "//trpc/config:config_factory",
        "//trpc/config:provider_factory",
        "//trpc/config/testing:codec_plugin_testing",
        "//trpc/config/testing:provider_plugin_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trpc_conf_compatible_test",
    srcs = ["trpc_conf_compatible_test.cc"],
    deps = [
        ":trpc_conf",
        "@com_github_fmtlib_fmt//:fmtlib",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "config_test",
    srcs = ["config_test.cc"],
    deps = [
        "//trpc/config/testing:mock_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)