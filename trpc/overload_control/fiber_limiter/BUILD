licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fiber_limiter_conf",
    srcs = ["fiber_limiter_conf.cc"],
    hdrs = ["fiber_limiter_conf.h"],
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
    name = "fiber_limiter_conf_test",
    srcs = ["fiber_limiter_conf_test.cc"],
    deps = [
        ":fiber_limiter_conf",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "fiber_limiter_client_filter",
    srcs = ["fiber_limiter_client_filter.cc"],
    hdrs = ["fiber_limiter_client_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":fiber_limiter_conf",
        "//trpc/client:client_context",
        "//trpc/common/config:trpc_config",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/log:trpc_log",
        "//trpc/metrics:trpc_metrics",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control/common:report",
        "//trpc/runtime:fiber_runtime",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_test(
    name = "fiber_limiter_client_filter_test",
    srcs = ["fiber_limiter_client_filter_test.cc"],
    data = ["fibers_overload_ctrl.yaml"],
    deps = [
        ":fiber_limiter_client_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common:runtime_manager",
        "//trpc/common:trpc_plugin",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine:fiber_basic",
        "//trpc/filter:filter_manager",
        "//trpc/transport/client/fiber/testing:thread_model_op",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "fiber_limiter_server_filter",
    srcs = ["fiber_limiter_server_filter.cc"],
    hdrs = ["fiber_limiter_server_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":fiber_limiter_conf",
        "//trpc/common/config:trpc_config",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/log:trpc_log",
        "//trpc/metrics:trpc_metrics",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control/common:report",
        "//trpc/runtime:fiber_runtime",
        "//trpc/server:server_context",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_test(
    name = "fiber_limiter_server_filter_test",
    srcs = ["fiber_limiter_server_filter_test.cc"],
    data = ["fibers_overload_ctrl.yaml"],
    deps = [
        ":fiber_limiter_server_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common:trpc_plugin",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine:fiber_basic",
        "//trpc/filter:filter_manager",
        "//trpc/transport/client/fiber/testing:thread_model_op",
        "@com_google_googletest//:gtest_main",
    ],
)
