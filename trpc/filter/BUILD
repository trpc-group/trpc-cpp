load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "filter_point",
    hdrs = ["filter_point.h"],
    deps = [],
)

cc_library(
    name = "filter_id_counter",
    srcs = ["filter_id_counter.cc"],
    hdrs = ["filter_id_counter.h"],
    deps = [
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "filter_id_counter_test",
    srcs = ["filter_id_counter_test.cc"],
    deps = [
        ":filter_id_counter",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "filter_base",
    hdrs = ["filter_base.h"],
    deps = [
        ":filter_id_counter",
        ":filter_point",
        "//trpc/common:plugin",
        "//trpc/util:ref_ptr",
    ],
)

cc_test(
    name = "filter_base_test",
    srcs = ["filter_base_test.cc"],
    deps = [
        ":filter_base",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_filter_base",
    hdrs = ["server_filter_base.h"],
    deps = [
        ":filter_base",
        "//trpc/common:status",
    ],
)

cc_library(
    name = "server_filter_controller_h",
    hdrs = ["server_filter_controller.h"],
    deps = [
        ":server_filter_base",
    ],
)

cc_library(
    name = "server_filter_controller",
    srcs = ["server_filter_controller.cc"],
    deps = [
        ":server_filter_controller_h",
        ":server_filter_manager",
        "//trpc/server:server_context_h",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "client_filter_base",
    hdrs = ["client_filter_base.h"],
    deps = [
        ":filter_base",
        "//trpc/common:status",
    ],
)

cc_library(
    name = "client_filter_controller",
    srcs = ["client_filter_controller.cc"],
    deps = [
        ":client_filter_controller_h",
        ":client_filter_manager",
        "//trpc/client:client_context",
    ],
)

cc_test(
    name = "client_filter_controller_test",
    srcs = ["client_filter_controller_test.cc"],
    deps = [
        ":client_filter_controller",
        ":client_filter_manager",
        "//trpc/client:client_context",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/filter:filter_controller",
        "//trpc/filter/testing:client_filter_testing",
    ],
)

cc_library(
    name = "client_filter_manager",
    srcs = ["client_filter_manager.cc"],
    hdrs = ["client_filter_manager.h"],
    deps = [
        ":client_filter_base",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "client_filter_manager_test",
    srcs = ["client_filter_manager_test.cc"],
    deps = [
        ":client_filter_manager",
        "//trpc/client:client_context",
        "//trpc/filter:filter_controller",
        "//trpc/filter/testing:client_filter_testing",
    ],
)

cc_library(
    name = "client_filter_controller_h",
    hdrs = ["client_filter_controller.h"],
    deps = [
        ":client_filter_base",
    ],
)

cc_test(
    name = "server_filter_controller_test",
    srcs = ["server_filter_controller_test.cc"],
    deps = [
        ":server_filter_controller",
        ":server_filter_manager",
        "//trpc/filter:filter_controller",
        "//trpc/filter/testing:server_filter_testing",
        "//trpc/server:server_context",
    ],
)

cc_library(
    name = "server_filter_manager",
    srcs = ["server_filter_manager.cc"],
    hdrs = ["server_filter_manager.h"],
    deps = [
        ":server_filter_base",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "server_filter_manager_test",
    srcs = ["server_filter_manager_test.cc"],
    deps = [
        ":server_filter_manager",
        "//trpc/filter/testing:server_filter_testing",
    ],
)

cc_library(
    name = "filter",
    hdrs = ["filter.h"],
    deps = [
        ":client_filter_base",
        ":server_filter_base",
    ],
)

cc_test(
    name = "filter_test",
    srcs = ["filter_test.cc"],
    deps = [
        ":filter",
        ":filter_controller",
        ":filter_manager",
        "//trpc/client:client_context",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/server:server_context",
        "//trpc/transport/common:transport_message",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "filter_controller_h",
    hdrs = ["filter_controller.h"],
    deps = [
        ":client_filter_controller_h",
        ":server_filter_controller_h",
    ],
)

cc_library(
    name = "filter_controller",
    srcs = ["filter_controller.cc"],
    deps = [
        ":client_filter_controller",
        ":filter_controller_h",
        ":server_filter_controller",
    ],
)

cc_test(
    name = "filter_controller_test",
    srcs = ["filter_controller_test.cc"],
    deps = [
        ":filter",
        ":filter_controller",
        ":filter_manager",
        "//trpc/client:client_context",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/filter/testing:client_filter_testing",
        "//trpc/filter/testing:server_filter_testing",
        "//trpc/server:server_context",
        "//trpc/transport/common:transport_message",
        "//trpc/util:ref_ptr",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "rpc_filter",
    srcs = ["rpc_filter.cc"],
    hdrs = ["rpc_filter.h"],
    deps = [
        ":filter",
        "//trpc/client:client_context",
        "//trpc/server:server_context",
        "//trpc/util/flatbuffers:fbs_interface",
        "//trpc/util/log:logging",
        "@com_github_tencent_rapidjson//:rapidjson",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "rpc_filter_test",
    srcs = ["rpc_filter_test.cc"],
    deps = [
        ":rpc_filter",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/proto/testing:helloworld_fbs",
        "//trpc/util/flatbuffers:fbs_interface",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "filter_manager",
    srcs = ["filter_manager.cc"],
    hdrs = ["filter_manager.h"],
    deps = [
        ":client_filter_manager",
        ":filter",
        ":server_filter_manager",
    ],
)

cc_library(
    name = "trpc_filter",
    srcs = ["trpc_filter.cc"],
    hdrs = ["trpc_filter.h"],
    defines = [] + select({
                  "//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
                  "//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
                  "//conditions:default": [],
              }) +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }) + select({
        "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
        "//conditions:default": [],
    }),
    deps = [
               ":filter_manager",
               "//trpc/client:make_client_context",
               "//trpc/common/config:trpc_config",
               #"//trpc/filter/retry:retry_limit_client_filter",
           ] + select({
               "//conditions:default": [],
               "//trpc:trpc_include_prometheus": [
                   "//trpc/metrics/prometheus:prometheus_client_filter",
                   "//trpc/metrics/prometheus:prometheus_server_filter",
               ],
               "//trpc:include_metrics_prometheus": [
                   "//trpc/metrics/prometheus:prometheus_client_filter",
                   "//trpc/metrics/prometheus:prometheus_server_filter",
               ],
           }) + select({
               "//conditions:default": [],
               "//trpc:trpc_include_rpcz": [
                   "//trpc/rpcz/filter:client_filter",
                   "//trpc/rpcz/filter:server_filter",
                   "//trpc/rpcz:span",
               ],
           }) +
           select({
               "//conditions:default": [],
               "//trpc:trpc_include_overload_control": [
                   "//trpc/overload_control/flow_control:flow_controller_server_filter",
                   "//trpc/overload_control/concurrency_limiter:concurrency_limiter_server_filter",
                   "//trpc/overload_control/fiber_limiter:fiber_limiter_client_filter",
                   "//trpc/overload_control/fiber_limiter:fiber_limiter_server_filter",
                   "//trpc/overload_control/high_percentile:high_percentile_server_filter",
                   "//trpc/overload_control/throttler:throttler_client_filter",
               ],
           }),
)
