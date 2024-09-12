licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "seconds_overload_controller",
    srcs = ["seconds_overload_controller.cc"],
    hdrs = ["seconds_overload_controller.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/overload_control/common:report",
        "//trpc/util:time",
        "//trpc/util/log:logging",
        "//trpc/overload_control:server_overload_controller",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control/flow_control:seconds_limiter",
        "//trpc/overload_control/flow_control:flow_controller_conf",
        "//trpc/overload_control/flow_control:flow_controller_generator",
    ],
)

cc_test(
	name = "seconds_overload_controller_test",
	srcs = ["seconds_overload_controller_test.cc"],
	deps = [
		":seconds_overload_controller",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/overload_control/flow_control:flow_controller_conf",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
	],
)

cc_library(
    name = "seconds_limiter_server_filter",
    srcs = ["seconds_limiter_server_filter.cc"],
    hdrs = ["seconds_limiter_server_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":seconds_overload_controller",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/log:trpc_log",
        "//trpc/server:server_context",
        "//trpc/util:likely",
        "//trpc/overload_control/flow_control:flow_controller_conf",
    ],
)

cc_test(
    name = "seconds_limiter_server_filter_test",
    srcs = ["seconds_limiter_server_filter_test.cc"],
    data = [
        "//trpc/overload_control/seconds_limiter:filter_test.yaml",
    ],
    deps = [
        ":seconds_overload_controller",
        ":seconds_limiter_server_filter",
        "//trpc/common:trpc_plugin",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/overload_control/flow_control:flow_controller_conf",
        "@com_google_googletest//:gtest_main",
    ],
)