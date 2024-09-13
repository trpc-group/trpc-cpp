licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "window_limiter_overload_controller_filter",
    srcs = ["window_limiter_overload_controller_filter.cc"],
    hdrs = ["window_limiter_overload_controller_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":window_limiter_overload_controller",
        "//trpc/filter:filter",
        "//trpc/overload_control/flow_control:flow_controller_conf",
        "//trpc/server:server_context",
        "//trpc/util:likely",
    ],
)

cc_library(
    name = "window_limiter_overload_controller",
    srcs = ["window_limiter_overload_controller.cc"],
    hdrs = ["window_limiter_overload_controller.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control:server_overload_controller",
        "//trpc/overload_control/common:report",
        "//trpc/overload_control/flow_control:flow_controller",
        "//trpc/overload_control/flow_control:flow_controller_conf",
        "//trpc/overload_control/flow_control:flow_controller_generator",
        "//trpc/overload_control/flow_control:smooth_limiter",
        "//trpc/common/config:trpc_config",
        "//trpc/log:trpc_log",
    ],
)

cc_test(
    name = "window_limiter_overload_controller_filter_test",
    srcs = ["window_limiter_overload_controller_filter_test.cc"],
    data = [
        ":filter_test.yaml",
    ],
    deps = [
        ":window_limiter_overload_controller",
        ":window_limiter_overload_controller_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common:trpc_plugin",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "window_limiter_overload_controller_test",
    srcs = ["window_limiter_overload_controller_test.cc"],
    data = [
        ":filter_test.yaml",
    ],
    deps = [
        ":window_limiter_overload_controller",
        ":window_limiter_overload_controller_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common:trpc_plugin",
        "@com_google_googletest//:gtest_main",
    ],
)
