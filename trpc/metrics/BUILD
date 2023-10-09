licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "metrics",
    hdrs = ["metrics.h"],
    deps = [
        "//trpc/common:plugin",
    ],
)

cc_library(
    name = "metrics_factory",
    srcs = ["metrics_factory.cc"],
    hdrs = ["metrics_factory.h"],
    deps = [
        ":metrics",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "trpc_metrics_report",
    srcs = ["trpc_metrics_report.cc"],
    hdrs = ["trpc_metrics_report.h"],
    deps = [
        ":metrics",
        ":metrics_factory",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "trpc_metrics_deprecated",
    srcs = ["trpc_metrics_deprecated.cc"],
    hdrs = ["trpc_metrics_deprecated.h"],
    deps = [
        ":trpc_metrics_report",
    ],
)

cc_library(
    name = "trpc_metrics",
    srcs = ["trpc_metrics.cc"],
    hdrs = ["trpc_metrics.h"],
    defines = [] + select({
        "//trpc:trpc_include_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//trpc:include_metrics_prometheus": ["TRPC_BUILD_INCLUDE_PROMETHEUS"],
        "//conditions:default": [],
    }),
    deps = [
        ":metrics",
        ":metrics_factory",
        ":trpc_metrics_deprecated",
        ":trpc_metrics_report",
        "//trpc/util/log:logging",
    ] + select({
        "//conditions:default": [],
        "//trpc:trpc_include_prometheus": [
            "//trpc/metrics/prometheus:prometheus_metrics",
        ],
        "//trpc:include_metrics_prometheus": [
            "//trpc/metrics/prometheus:prometheus_metrics",
        ],
    }),
)

cc_test(
    name = "metrics_factory_test",
    srcs = ["metrics_factory_test.cc"],
    deps = [
        ":metrics_factory",
        "//trpc/metrics/testing:metrics_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trpc_metrics_report_test",
    srcs = ["trpc_metrics_report_test.cc"],
    deps = [
        ":trpc_metrics_report",
        "//trpc/metrics/testing:metrics_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trpc_metrics_test",
    srcs = ["trpc_metrics_test.cc"],
    data = ["//trpc/metrics/testing:metrics.yaml"],
    deps = [
        ":trpc_metrics",
        "//trpc/common/config:trpc_config",
        "//trpc/metrics/testing:metrics_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
