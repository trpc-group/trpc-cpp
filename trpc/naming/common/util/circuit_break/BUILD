licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "bucket_circular_array",
    srcs = [
        "bucket_circular_array.cc",
    ],
    hdrs = [
        "bucket_circular_array.h",
    ],
    deps = [
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "circuit_break_whitelist",
    srcs = [
        "circuit_break_whitelist.cc",
    ],
    hdrs = [
        "circuit_break_whitelist.h",
    ],
    deps = [
        "//trpc/codec/trpc",
    ],
)

cc_test(
    name = "circuit_break_whitelist_test",
    srcs = [
        "circuit_break_whitelist_test.cc",
    ],
    deps = [
        ":circuit_break_whitelist",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "circuit_breaker_config",
    hdrs = [
        "circuit_breaker_config.h",
    ],
    deps = [
        "//trpc/util/log:logging",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "default_circuit_breaker_config",
    hdrs = [
        "default_circuit_breaker_config.h",
    ],
    deps = [
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "default_circuit_breaker",
    srcs = [
        "default_circuit_breaker.cc",
    ],
    hdrs = [
        "default_circuit_breaker.h",
    ],
    deps = [
        ":bucket_circular_array",
        ":circuit_breaker",
        ":default_circuit_breaker_config",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "circuit_breaker",
    srcs = [],
    hdrs = [
        "circuit_breaker.h",
    ],
    deps = [
        "//trpc/naming/common:common_defs",
    ],
)

cc_library(
    name = "circuit_breaker_creator_factory",
    srcs = [],
    hdrs = [
        "circuit_breaker_creator_factory.h",
    ],
    deps = [
        ":circuit_breaker",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
    name = "default_circuit_breaker_test",
    srcs = ["default_circuit_breaker_test.cc"],
    deps = [
        ":default_circuit_breaker",
        "//trpc/util:time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "default_circuit_beaker_config_test",
    srcs = ["default_circuit_beaker_config_test.cc"],
    deps = [
        ":default_circuit_breaker_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
