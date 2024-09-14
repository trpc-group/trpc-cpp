licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "sliding_window_limiter_conf",
    srcs = ["sliding_window_limiter_conf.cc"],
    hdrs = ["sliding_window_limiter_conf.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/log:trpc_log",
        "//trpc/overload_control:overload_control_defs",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
    name = "sliding_window_limiter_conf_test",
    srcs = ["sliding_window_limiter_conf_test.cc"],
    deps = [
        ":sliding_window_limiter_conf",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "sliding_window_overload_controller",
    srcs = ["sliding_window_overload_controller.cc"],
    hdrs = ["sliding_window_overload_controller.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":sliding_window_limiter_conf",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control:server_overload_controller",
        "//trpc/overload_control/flow_control:smooth_limiter",
    ],
)

cc_test(
    name = "sliding_window_overload_controller_test",
    srcs = ["sliding_window_overload_controller_test.cc"],
    deps = [
        ":sliding_window_limiter_conf",
        ":sliding_window_overload_controller",
        "//trpc/codec/testing:protocol_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "sliding_window_limiter_server_filter",
    srcs = ["sliding_window_limiter_server_filter.cc"],
    hdrs = ["sliding_window_limiter_server_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":sliding_window_limiter_conf",
        ":sliding_window_overload_controller",
        "//trpc/filter",
        "//trpc/log:trpc_log",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control/common:report",
        "//trpc/server:server_context",
        "//trpc/server:service_impl",
        "//trpc/util:ref_ptr",
    ],
)

cc_test(
    name = "sliding_window_limiter_server_filter_test",
    srcs = ["sliding_window_limiter_server_filter_test.cc"],
    data = ["sliding_window_overload_ctrl.yaml"],
    deps = [
        ":sliding_window_limiter_server_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common/config:trpc_config",
        "//trpc/filter:filter_manager",
        "@com_google_googletest//:gtest_main",
    ],
)
