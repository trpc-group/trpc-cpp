# Description: trpc-cpp.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "high_avg_ema",
    srcs = ["high_avg_ema.cc"],
    hdrs = ["high_avg_ema.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
)

cc_test(
    name = "high_avg_ema_test",
    srcs = ["high_avg_ema_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":high_avg_ema",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "high_avg_strategy",
    srcs = ["high_avg_strategy.cc"],
    hdrs = ["high_avg_strategy.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":high_avg",
    ],
)

cc_test(
    name = "high_avg_strategy_test",
    srcs = ["high_avg_strategy_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":high_avg_strategy",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "high_avg",
    srcs = ["high_avg.cc"],
    hdrs = ["high_avg.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":max_in_every_n",
        "//trpc/common/logging:trpc_logging",
        "//trpc/util:check",
        "//trpc/util/thread:spinlock",
    ],
)

cc_test(
    name = "high_avg_test",
    srcs = ["high_avg_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":high_avg",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "high_percentile_conf",
    srcs = ["high_percentile_conf.cc"],
    hdrs = ["high_percentile_conf.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/log:trpc_log",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control/common:priority_conf_parse",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
    name = "high_percentile_conf_test",
    srcs = ["high_percentile_conf_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":high_percentile_conf",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "high_percentile_priority_impl",
    srcs = ["high_percentile_priority_impl.cc"],
    hdrs = ["high_percentile_priority_impl.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":high_avg_ema",
        ":high_avg_strategy",
        "//trpc/log:trpc_log",
        "//trpc/overload_control/common:priority",
        "//trpc/overload_control/common:report",
        "//trpc/overload_control/common:window",
        "//trpc/util/thread:spinlock",
    ],
)

cc_test(
    name = "high_percentile_priority_impl_test",
    srcs = ["high_percentile_priority_impl_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":high_percentile_priority_impl",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "high_percentile_overload_controller",
    srcs = ["high_percentile_overload_controller.cc"],
    hdrs = ["high_percentile_overload_controller.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":high_percentile_priority_impl",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control/common:priority_adapter",
        "//trpc/overload_control/common:request_priority",
    ],
)

cc_test(
    name = "high_percentile_overload_controller_test",
    srcs = ["high_percentile_overload_controller_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":high_percentile_overload_controller",
        "//trpc/codec/testing:protocol_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "high_percentile_server_filter",
    srcs = ["high_percentile_server_filter.cc"],
    hdrs = ["high_percentile_server_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":high_percentile_conf",
        ":high_percentile_overload_controller",
        "//trpc/filter",
        "//trpc/overload_control:overload_control_defs",
    ],
)

cc_test(
    name = "high_percentile_server_filter_test",
    srcs = ["high_percentile_server_filter_test.cc"],
    data = [":high_percentile.yaml"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":high_percentile_server_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common:trpc_plugin",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "max_in_every_n",
    srcs = ["max_in_every_n.cc"],
    hdrs = ["max_in_every_n.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
)

cc_test(
    name = "max_in_every_n_test",
    srcs = ["max_in_every_n_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":max_in_every_n",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
