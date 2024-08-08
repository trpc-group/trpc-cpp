licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "seconds_limiter_conf",
    srcs = ["seconds_limiter_conf.cc"],
    hdrs = ["seconds_limiter_conf.h"],
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
    name = "seconds_limiter_conf_test",
    srcs = ["seconds_limiter_conf_test.cc"],
    deps = [
        ":seconds_limiter_conf",
        "@com_google_googletest//:gtest_main",
    ],
)
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
        "//trpc/overload_control/flow_control:seconds_limiter"

    ],
)
cc_test(
	name = "seconds_overload_controller_test",
	srcs = ["seconds_overload_controller_test.cc"],
	deps = [
		":seconds_overload_controller",
        "//trpc/codec/trpc:trpc_protocol",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
	],
)
cc_library(
    name = "seconds_overload_controller_factory",
    hdrs = ["seconds_overload_controller_factory.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/overload_control/common:overload_control_factory",
        "//trpc/overload_control:server_overload_controller",
    ],
)
cc_library(
    name = "seconds_limiter_generator",
    srcs = ["seconds_limiter_generator.cc"],
    hdrs = ["seconds_limiter_generator.h"],
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
        ":seconds_limiter_conf",
        ":seconds_overload_controller_factory",
        "//trpc/common/logging:trpc_logging",
        "//trpc/util:function",
        "//trpc/util:string_util",
    ],
)
cc_test(
    name = "seconds_limiter_generator_test",
    srcs = ["seconds_limiter_generator_test.cc"],
    deps = [
        ":seconds_overload_controller",
        ":seconds_overload_controller_factory",
        ":seconds_limiter_generator",
        ":seconds_limiter_conf",
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
        ":seconds_limiter_conf",
        ":seconds_overload_controller_factory",
        ":seconds_limiter_generator",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/log:trpc_log",
        "//trpc/server:server_context",
        "//trpc/util:likely",
    ],
)
cc_test(
    name = "seconds_limiter_server_filter_test",
    srcs = ["seconds_limiter_server_filter_test.cc"],
    data = [
        "//trpc/overload_control/seconds_limiter:filter_test.yaml",
    ],
    deps = [
        "//trpc/common:trpc_plugin",
        ":seconds_overload_controller_factory",
        ":seconds_limiter_server_filter",
        ":seconds_overload_controller",
        "//trpc/codec/testing:protocol_testing",
        "@com_google_googletest//:gtest_main",
    ],
)