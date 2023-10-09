# Description: trpc-cpp.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//trpc/overload_control:__subpackages__"])

cc_library(
    name = "histogram",
    srcs = ["histogram.cc"],
    hdrs = ["histogram.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/util:check",
    ],
)

cc_test(
    name = "histogram_test",
    srcs = ["histogram_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":histogram",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "priority_adapter",
    srcs = ["priority_adapter.cc"],
    hdrs = ["priority_adapter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":histogram",
        ":priority",
        ":report",
        ":window",
        "//trpc/util:random",
        "//trpc/util/thread:spinlock",
    ],
)

cc_test(
    name = "priority_adapter_test",
    srcs = ["priority_adapter_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":priority_adapter",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "priority_conf_parse",
    hdrs = ["priority_conf_parse.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "priority",
    hdrs = ["priority.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
)

cc_library(
    name = "report",
    srcs = ["report.cc"],
    hdrs = ["report.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/metrics:trpc_metrics",
        "//trpc/overload_control:overload_control_defs",
    ],
)

cc_test(
    name = "report_test",
    srcs = ["report_test.cc"],
    data = [":report_test.yaml"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":report",
        "//trpc/common/config:trpc_config",
        "//trpc/metrics:metrics_factory",
        "//trpc/metrics:trpc_metrics",
        "//trpc/metrics/testing:metrics_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "window",
    srcs = ["window.cc"],
    hdrs = ["window.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/util:check",
    ],
)

cc_library(
    name = "request_priority",
    srcs = ["request_priority.cc"],
    hdrs = ["request_priority.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/client:client_context",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/server:server_context",
        "//trpc/util:function",
    ],
)

cc_test(
    name = "request_priority_test",
    srcs = ["request_priority_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":request_priority",
        "//trpc/codec/trpc:trpc_protocol",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "window_test",
    srcs = ["window_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":window",
        "//trpc/transport/server:server_transport_message",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
