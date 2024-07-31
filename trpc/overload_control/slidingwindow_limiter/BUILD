licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "slidingwindow_limiter_conf",
    srcs = ["slidingwindow_limiter_conf.cc"],
    hdrs = ["slidingwindow_limiter_conf.h"],
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
    name = "slidingwindow_limiter_conf_test",
    srcs = ["slidingwindow_limiter_conf_test.cc"],
    deps = [
        ":slidingwindow_limiter_conf",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "slidingwindow_overload_controller",
    srcs = ["slidingwindow_overload_controller.cc"],
    hdrs = ["slidingwindow_overload_controller.h"],
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
    name = "slidingwindow_overload_controller_test",
    srcs = ["slidingwindow_overload_controller_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
		":slidingwindow_overload_controller",
        "//trpc/codec/trpc:trpc_protocol",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
	],
)

cc_library(
    name = "slidingwindow_limiter_server_filter",
    srcs = ["slidingwindow_limiter_server_filter.cc"],
    hdrs = ["slidingwindow_limiter_server_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":slidingwindow_limiter_conf",
        ":slidingwindow_overload_controller",
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
    name = "slidingwindow_limiter_server_filter_test",
    srcs = ["slidingwindow_limiter_server_filter_test.cc"],
    data = ["slidingwindow_overload_ctrl.yaml"],
    deps = [
        ":slidingwindow_limiter_server_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common:trpc_plugin",
        "//trpc/filter:filter_manager",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest_main",
    ],
)