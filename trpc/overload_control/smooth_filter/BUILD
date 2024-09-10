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
        "//trpc/overload_control/flow_control:flow_controller_conf",
        ":server_overload_controller_factory",
        ":server_smooth_limit",
        "//trpc/common/logging:trpc_logging",
        "//trpc/util:function",
        "//trpc/util:string_util",
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
        "//trpc/overload_control/flow_control:flow_controller_conf",
        ":server_overload_controller_factory",
        ":server_flow_controller_generator",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/log:trpc_log",
        "//trpc/server:server_context",
        "//trpc/util:likely",
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
        "//trpc/overload_control/flow_control:smooth_limiter",
        ":server_overload_controller",
        "//trpc/overload_control/common:report",
        "//trpc/util/log:logging",
    ],
)

