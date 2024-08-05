licenses(["notice"])

package(default_visibility = ["//visibility:public"])





cc_test(
    name = "server_overload_controller_factory_test",
    srcs = ["server_overload_controller_factory_test.cc"],
    deps = [
        ":server_overload_controller_factory",
        "//trpc/overload_control/testing:overload_control_testing",
        "@com_google_googletest//:gtest_main",
    ],
)



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
    name = "server_overload_controller_factory",
    hdrs = ["server_overload_controller_factory.h"],
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
   name = "trpc_overload_control",
   srcs = ["trpc_overload_control.cc"],
   hdrs = ["trpc_overload_control.h"],
   deps = [
       ":server_overload_controller_factory",
       "//trpc/filter:filter_manager",
   ],
)

cc_library(
    name = "server_flow_controller_conf",
    srcs = ["server_flow_controller_conf.cc"],
    hdrs = ["server_flow_controller_conf.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/log:trpc_log",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
    name = "server_flow_controller_conf_test",
    srcs = ["server_flow_controller_conf_test.cc"],
    deps = [
        ":server_flow_controller_conf",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_flow_controller_generator",
    srcs = ["server_flow_controller_generator.cc"],
    hdrs = ["server_flow_controller_generator.h"],
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
        ":server_flow_controller_conf",
        ":server_overload_controller_factory",
        ":server_smooth_limit",
        "//trpc/common/logging:trpc_logging",
        "//trpc/util:function",
        "//trpc/util:string_util",
    ],
)

cc_test(
    name = "server_flow_controller_generator_test",
    srcs = ["server_flow_controller_generator_test.cc"],
    deps = [
        ":server_smooth_limit",
        ":server_overload_controller_factory",
        ":server_flow_controller_generator",
        ":server_flow_controller_conf",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_flow_controller_server_filter",
    srcs = ["server_flow_controller_server_filter.cc"],
    hdrs = ["server_flow_controller_server_filter.h"],
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
        ":server_flow_controller_conf",
        ":server_overload_controller_factory",
        ":server_flow_controller_generator",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/log:trpc_log",
        "//trpc/server:server_context",
        "//trpc/util:likely",
    ],
)

cc_test(
    name = "server_flow_controller_server_filter_test",
    srcs = ["server_flow_controller_server_filter_test.cc"],
    data = [
        "//trpc/overload_control/smooth_filter:filter_test.yaml",
    ],
    deps = [
        "//trpc/common:trpc_plugin",
        ":server_overload_controller_factory",
        ":server_flow_controller_server_filter",
        "//trpc/codec/testing:protocol_testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_request_roll_queue",
    srcs = ["server_request_roll_queue.cc"],
    hdrs = ["server_request_roll_queue.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:private"],
    deps = [
        "//trpc/util/log:logging",
        ]
)

cc_test(
    name = "server_request_roll_queue_test",
    srcs = ["server_request_roll_queue_test.cc"],
    deps = [
        "//trpc/log:trpc_log",
        ":server_request_roll_queue",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_smooth_limit",
    srcs = ["server_smooth_limit.cc"],
    hdrs = ["server_smooth_limit.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":server_request_roll_queue",
        ":server_overload_controller",
        ":server_trick_time",
        "//trpc/overload_control/common:report",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "server_smooth_limit_test",
    srcs = ["server_smooth_limit_test.cc"],
    deps = [
        ":server_smooth_limit",
        "//trpc/codec/trpc:trpc_protocol",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_trick_time",
    srcs = ["server_trick_time.cc"],
    hdrs = ["server_trick_time.h"],
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
    name = "server_trick_time_test",
    srcs = ["server_trick_time_test.cc"],
    deps = [
        ":server_trick_time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)