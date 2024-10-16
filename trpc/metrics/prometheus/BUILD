licenses(["notice"])

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "test_yaml_files",
    srcs = glob([
        "testing/*.yaml",
    ]),
)

cc_library(
    name = "prometheus_pusher",
    srcs = ["prometheus_pusher.cc"],
    hdrs = ["prometheus_pusher.h"],
    deps = [
        "//trpc/util/log:logging",
        "@com_github_jupp0r_prometheus_cpp//push",
    ],
)

cc_library(
    name = "prometheus_conf",
    srcs = ["prometheus_conf.cc"],
    hdrs = ["prometheus_conf.h"],
    deps = [
        "//trpc/util/log:logging",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "prometheus_conf_parser",
    hdrs = ["prometheus_conf_parser.h"],
    deps = [
        ":prometheus_conf",
    ],
)

cc_test(
    name = "prometheus_conf_test",
    srcs = ["prometheus_conf_test.cc"],
    deps = [
        ":prometheus_conf",
        ":prometheus_conf_parser",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "prometheus_common",
    srcs = ["prometheus_common.cc"],
    hdrs = ["prometheus_common.h"],
    defines = [] + select({
        "//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [],
)

cc_test(
    name = "prometheus_common_test",
    srcs = ["prometheus_common_test.cc"],
    deps = [
        ":prometheus_common",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "prometheus_metrics",
    srcs = ["prometheus_metrics.cc"],
    hdrs = ["prometheus_metrics.h"],
    defines = [] + select({
        "//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [
        ":prometheus_common",
        ":prometheus_conf",
        ":prometheus_conf_parser",
        "//trpc/util:prometheus",
        "@com_github_jupp0r_prometheus_cpp//core",

        "//trpc/common/config:trpc_config",
        ":prometheus_pusher",
        "//trpc/metrics",
    ] + select({
        "//conditions:default": [],
        "//trpc:trpc_include_prometheus": [
            "@com_github_jupp0r_prometheus_cpp//pull",
            "@com_github_jupp0r_prometheus_cpp//push",
        ],
        "//trpc:include_metrics_prometheus": [
            "@com_github_jupp0r_prometheus_cpp//pull",
            "@com_github_jupp0r_prometheus_cpp//push",
        ],
    }),
)

cc_test(
    name = "prometheus_metrics_test",
    srcs = ["prometheus_metrics_test.cc"],
    data = [":test_yaml_files"],
    deps = [
        ":prometheus_metrics",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "prometheus_client_filter",
    srcs = ["prometheus_client_filter.cc"],
    hdrs = ["prometheus_client_filter.h"],
    defines = [] + select({
        "//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [
        ":prometheus_common",
        "//trpc/client:client_context",
        "//trpc/common/config:trpc_config",
        "//trpc/filter",
        "//trpc/metrics",
        "//trpc/metrics:metrics_factory",
        "//trpc/util:time",
    ],
)

cc_test(
    name = "prometheus_client_filter_test",
    srcs = ["prometheus_client_filter_test.cc"],
    data = [":test_yaml_files"],
    deps = [
        ":prometheus_client_filter",
        ":prometheus_metrics",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/common/config:trpc_config",
        "//trpc/metrics:trpc_metrics",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "prometheus_server_filter",
    srcs = ["prometheus_server_filter.cc"],
    hdrs = ["prometheus_server_filter.h"],
    defines = [] + select({
        "//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [
        ":prometheus_common",
        "//trpc/common/config:trpc_config",
        "//trpc/filter",
        "//trpc/metrics",
        "//trpc/metrics:metrics_factory",
        "//trpc/server:server_context",
        "//trpc/util:time",
    ],
)

cc_test(
    name = "prometheus_server_filter_test",
    srcs = ["prometheus_server_filter_test.cc"],
    data = [":test_yaml_files"],
    deps = [
        ":prometheus_metrics",
        ":prometheus_server_filter",
        "//trpc/codec:codec_manager",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/common/config:trpc_config",
        "//trpc/metrics:trpc_metrics",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server/rpc:rpc_service_impl",
        "//trpc/server/testing:server_context_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "prometheus_metrics_api",
    srcs = ["prometheus_metrics_api.cc"],
    hdrs = ["prometheus_metrics_api.h"],
    defines = [] + select({
        "//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [
        ":prometheus_common",
        ":prometheus_metrics",
        "//trpc/metrics",
        "//trpc/metrics:metrics_factory",
        "//trpc/metrics:trpc_metrics",
        "//trpc/util:prometheus",
    ],
)

cc_test(
    name = "prometheus_metrics_api_test",
    srcs = ["prometheus_metrics_api_test.cc"],
    data = [":test_yaml_files"],
    deps = [
        ":prometheus_metrics_api",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
