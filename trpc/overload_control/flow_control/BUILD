# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "flow_controller_conf",
    srcs = ["flow_controller_conf.cc"],
    hdrs = ["flow_controller_conf.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/log:trpc_log",
        "//trpc/overload_control:overload_control_defs",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
    name = "flow_controller_conf_test",
    srcs = ["flow_controller_conf_test.cc"],
    deps = [
        ":flow_controller_conf",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "flow_controller_factory",
    srcs = ["flow_controller_factory.cc"],
    hdrs = ["flow_controller_factory.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":flow_controller",
    ],
)

cc_test(
    name = "flow_controller_factory_test",
    srcs = ["flow_controller_factory_test.cc"],
    deps = [
        ":flow_controller_factory",
        ":seconds_limiter",
        ":smooth_limiter",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "flow_controller_generator",
    srcs = ["flow_controller_generator.cc"],
    hdrs = ["flow_controller_generator.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":flow_controller",
        ":flow_controller_conf",
        ":flow_controller_factory",
        ":seconds_limiter",
        ":smooth_limiter",
        "//trpc/common/logging:trpc_logging",
        "//trpc/util:function",
        "//trpc/util:string_util",
    ],
)

cc_test(
    name = "flow_controller_generator_test",
    srcs = ["flow_controller_generator_test.cc"],
    deps = [
        ":flow_controller_factory",
        ":flow_controller_generator",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "flow_controller_server_filter",
    srcs = ["flow_controller_server_filter.cc"],
    hdrs = ["flow_controller_server_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":flow_controller_conf",
        ":flow_controller_factory",
        ":flow_controller_generator",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/log:trpc_log",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/server:server_context",
        "//trpc/util:likely",
    ],
)

cc_test(
    name = "flow_controller_server_filter_test",
    srcs = ["flow_controller_server_filter_test.cc"],
    data = [
        "//trpc/overload_control/flow_control:flow_test.yaml",
    ],
    deps = [
        ":flow_controller_factory",
        ":flow_controller_server_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common:trpc_plugin",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "flow_controller",
    hdrs = ["flow_controller.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/server:server_context",
    ],
)

cc_library(
    name = "hit_queue",
    srcs = ["hit_queue.cc"],
    hdrs = ["hit_queue.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:private"],
)

cc_test(
    name = "hit_queue_test",
    srcs = ["hit_queue_test.cc"],
    deps = [
        ":hit_queue",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "seconds_limiter",
    srcs = ["seconds_limiter.cc"],
    hdrs = ["seconds_limiter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":flow_controller",
        "//trpc/overload_control/common:report",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "seconds_limiter_test",
    srcs = ["seconds_limiter_test.cc"],
    deps = [
        ":seconds_limiter",
        "//trpc/codec/trpc:trpc_protocol",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "smooth_limiter",
    srcs = ["smooth_limiter.cc"],
    hdrs = ["smooth_limiter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":flow_controller",
        ":recent_queue",
        "//trpc/overload_control/common:report",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "smooth_limiter_test",
    srcs = ["smooth_limiter_test.cc"],
    deps = [
        ":smooth_limiter",
        "//trpc/codec/trpc:trpc_protocol",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "tick_timer",
    srcs = ["tick_timer.cc"],
    hdrs = ["tick_timer.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:private"],
    deps = [
        "//trpc/util:function",
    ],
)

cc_test(
    name = "tick_timer_test",
    srcs = ["tick_timer_test.cc"],
    deps = [
        ":tick_timer",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "recent_queue",
    srcs = ["recent_queue.cc"],
    hdrs = ["recent_queue.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:private"],
    deps = [
        "//trpc/util:time",
    ],
)

cc_test(
    name = "recent_queue_test",
    srcs = ["recent_queue_test.cc"],
    deps = [
        ":recent_queue",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
