licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fixedwindow_limiter_conf",
    srcs = ["fixedwindow_limiter_conf.cc"],
    hdrs = ["fixedwindow_limiter_conf.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/log:trpc_log",
        "//trpc/overload_control:overload_control_defs",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
    name = "fixedwindow_limiter_conf_test",
    srcs = ["fixedwindow_limiter_conf_test.cc"],
    deps = [
        ":fixedwindow_limiter_conf",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "fixedwindow_overload_controller",
    srcs = ["fixedwindow_overload_controller.cc"],
    hdrs = ["fixedwindow_overload_controller.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/log:trpc_log",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/util:time",
        "//trpc/overload_control:server_overload_controller",
    ],
)

cc_test(
    name = "fixedwindow_overload_controller_test",
    srcs = ["fixedwindow_overload_controller_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
		":fixedwindow_overload_controller",
        "//trpc/codec/trpc:trpc_protocol",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
	],
)

cc_library(
    name = "fixedwindow_limiter_server_filter",
    srcs = ["fixedwindow_limiter_server_filter.cc"],
    hdrs = ["fixedwindow_limiter_server_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":fixedwindow_limiter_conf",
        ":fixedwindow_overload_controller",
        "//trpc/common/config:trpc_config",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/log:trpc_log",
        "//trpc/metrics:trpc_metrics",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control/common:report",
        "//trpc/runtime/common/stats:frame_stats",
        "//trpc/server:server_context",
        "//trpc/server:service_impl",
        "@com_github_fmtlib_fmt//:fmtlib",
        "//trpc/util:ref_ptr",
    ],
)

cc_test(
    name = "fixedwindow_limiter_server_filter_test",
    srcs = ["fixedwindow_limiter_server_filter_test.cc"],
    data = ["fixedwindow_overload_ctrl.yaml"],
    deps = [
        ":fixedwindow_limiter_server_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common:trpc_plugin",
        "//trpc/filter:filter_manager",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest_main",
    ],
)