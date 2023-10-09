licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "concurrency_limiter_conf",
    srcs = ["concurrency_limiter_conf.cc"],
    hdrs = ["concurrency_limiter_conf.h"],
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
    name = "concurrency_limiter_conf_test",
    srcs = ["concurrency_limiter_conf_test.cc"],
    deps = [
        ":concurrency_limiter_conf",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "concurrency_limiter_server_filter",
    srcs = ["concurrency_limiter_server_filter.cc"],
    hdrs = ["concurrency_limiter_server_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":concurrency_limiter_conf",
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
        "//trpc/util:ref_ptr",
    ],
)

cc_test(
    name = "concurrency_limiter_server_filter_test",
    srcs = ["concurrency_limiter_server_filter_test.cc"],
    data = ["concurrency_overload_ctrl.yaml"],
    deps = [
        ":concurrency_limiter_server_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest_main",
    ],
)
