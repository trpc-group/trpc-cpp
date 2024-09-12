licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "server_overload_controller",
    hdrs = ["server_overload_controller.h"],
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
    name = "window_limit_overload_controller_filter",
    srcs = ["window_limit_overload_controller_filter.cc"],
    hdrs = ["window_limit_overload_controller_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":server_overload_controller",
        ":window_limit_overload_controller",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/log:trpc_log",
        "//trpc/overload_control/flow_control:flow_controller_conf",
        "//trpc/overload_control/flow_control:flow_controller_generator",
        "//trpc/server:server_context",
        "//trpc/util:likely",
    ],
)

cc_library(
    name = "window_limit_overload_controller",
    srcs = ["window_limit_overload_controller.cc"],
    hdrs = ["window_limit_overload_controller.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":server_overload_controller",
        "//trpc/overload_control/common:report",
        "//trpc/overload_control/flow_control:flow_controller",
        "//trpc/overload_control/flow_control:flow_controller_conf",
        "//trpc/overload_control/flow_control:flow_controller_generator",
        "//trpc/overload_control/flow_control:smooth_limiter",
        "//trpc/common/config:trpc_config",
        "//trpc/log:trpc_log",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "window_limit_overload_controller_filter_test",
    srcs = ["window_limit_overload_controller_filter_test.cc"],
    data = [
        ":filter_test.yaml",
    ],
    deps = [
        ":window_limit_overload_controller",
        ":window_limit_overload_controller_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common:trpc_plugin",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "window_limit_overload_controller_test",
    srcs = ["window_limit_overload_controller_test.cc"],
    data = [
        ":filter_test.yaml",
    ],
    deps = [
        ":window_limit_overload_controller",
        ":window_limit_overload_controller_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common:trpc_plugin",
        "@com_google_googletest//:gtest_main",
    ],
)
