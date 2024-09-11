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
    name = "overload_controller_filter",
    srcs = ["overload_controller_filter.cc"],
    hdrs = ["overload_controller_filter.h"],
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
        ":smooth_limit_overload_controller",
        "//trpc/overload_control/flow_control:flow_controller_conf",
        "//trpc/overload_control/flow_control:flow_controller_generator",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/log:trpc_log",
        "//trpc/server:server_context",
        "//trpc/util:likely",
    ],
)



cc_library(
    name = "smooth_limit_overload_controller",
    srcs = ["smooth_limit_overload_controller.cc"],
    hdrs = ["smooth_limit_overload_controller.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/overload_control/flow_control:smooth_limiter",
        "//trpc/overload_control/flow_control:flow_controller_conf",
        "//trpc/overload_control/flow_control:flow_controller_generator",
        "//trpc/overload_control/flow_control:flow_controller",
        ":server_overload_controller",
        "//trpc/overload_control/common:report",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "overload_controller_filter_test",
    srcs = ["overload_controller_filter_test.cc"],
    data = [
        ":filter_test.yaml",
    ],
    deps = [
        ":overload_controller_filter",
        ":smooth_limit_overload_controller",
        "//trpc/common:trpc_plugin",
        "//trpc/codec/testing:protocol_testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "smooth_limit_overload_controller_test",
    srcs = ["overload_controller_filter_test.cc"],
    data = [
        ":filter_test.yaml",
    ],
    deps = [
        ":overload_controller_filter",
        ":smooth_limit_overload_controller",
        "//trpc/common:trpc_plugin",
        "//trpc/codec/testing:protocol_testing",
        "@com_google_googletest//:gtest_main",
    ],
)
