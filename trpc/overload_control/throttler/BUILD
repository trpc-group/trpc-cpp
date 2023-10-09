# Description: trpc-cpp.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "throttler_conf",
    srcs = ["throttler_conf.cc"],
    hdrs = ["throttler_conf.h"],
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
    name = "throttler_conf_test",
    srcs = ["throttler_conf_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":throttler_conf",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "throttler_client_filter",
    srcs = ["throttler_client_filter.cc"],
    hdrs = ["throttler_client_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":throttler_conf",
        ":throttler_overload_controller",
        ":throttler_priority_impl",
        "//trpc/common:plugin",
        "//trpc/common/config:trpc_config",
        "//trpc/filter",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/runtime/common:periphery_task_scheduler",
    ],
)

cc_test(
    name = "throttler_client_filter_test",
    srcs = ["throttler_client_filter_test.cc"],
    data = [":throttler.yaml"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":throttler_client_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/filter:client_filter_manager",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "throttler_ema",
    srcs = ["throttler_ema.cc"],
    hdrs = ["throttler_ema.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
)

cc_test(
    name = "throttler_ema_test",
    srcs = ["throttler_ema_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":throttler_ema",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "throttler_priority_impl",
    srcs = ["throttler_priority_impl.cc"],
    hdrs = ["throttler_priority_impl.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":throttler",
        "//trpc/overload_control/common:priority",
    ],
)

cc_test(
    name = "throttler_priority_impl_test",
    srcs = ["throttler_priority_impl_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":throttler_priority_impl",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "throttler_overload_controller",
    srcs = ["throttler_overload_controller.cc"],
    hdrs = ["throttler_overload_controller.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":throttler",
        ":throttler_priority_impl",
        "//trpc/client:client_context",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control/common:priority_adapter",
        "//trpc/overload_control/common:request_priority",
        "//trpc/util:time",
    ],
)

cc_test(
    name = "throttler_overload_controller_test",
    srcs = ["throttler_overload_controller_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":throttler_overload_controller",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/overload_control/common:request_priority",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "throttler",
    srcs = ["throttler.cc"],
    hdrs = ["throttler.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":throttler_ema",
        "//trpc/util:random",
        "//trpc/util/thread:spinlock",
    ],
)

cc_test(
    name = "throttler_test",
    srcs = ["throttler_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":throttler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
