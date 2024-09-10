licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "token_bucket_limiter_conf",
    srcs = ["token_bucket_limiter_conf.cc"],
    hdrs = ["token_bucket_limiter_conf.h"],
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
    name = "token_bucket_limiter_conf_test",
    srcs = ["token_bucket_limiter_conf_test.cc"],
    deps = [
        ":token_bucket_limiter_conf",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "token_bucket_overload_controller",
    srcs = ["token_bucket_overload_controller.cc"],
    hdrs = ["token_bucket_overload_controller.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
		":token_bucket_limiter_conf",
        "//trpc/log:trpc_log",
		"//trpc/tvar/common:atomic_type",
        "//trpc/overload_control:overload_control_defs",
        "//trpc/overload_control:server_overload_controller",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
	name = "token_bucket_overload_controller_test",
	srcs = ["token_bucket_overload_controller_test.cc"],
	deps = [
        ":token_bucket_limiter_conf",
		":token_bucket_overload_controller",
        "//trpc/codec/testing:protocol_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
	],
)

cc_library(
    name = "token_bucket_limiter_server_filter",
    srcs = ["token_bucket_limiter_server_filter.cc"],
    hdrs = ["token_bucket_limiter_server_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
    deps = [
        ":token_bucket_limiter_conf",
		":token_bucket_overload_controller",
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
	name = "token_bucket_limiter_server_filter_test",
	srcs = ["token_bucket_limiter_server_filter_test.cc"],
	data = ["token_bucket_overload_ctrl.yaml"],
	deps = [
		":token_bucket_limiter_server_filter",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest_main",
	],
)
