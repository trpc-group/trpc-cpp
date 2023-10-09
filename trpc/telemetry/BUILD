licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "telemetry",
    hdrs = ["telemetry.h"],
    deps = [
        "//trpc/common:plugin",
        "//trpc/log:logging",
        "//trpc/metrics",
        "//trpc/tracing",
    ],
)

cc_library(
    name = "telemetry_factory",
    srcs = ["telemetry_factory.cc"],
    hdrs = ["telemetry_factory.h"],
    deps = [
        ":telemetry",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "trpc_telemetry",
    srcs = ["trpc_telemetry.cc"],
    hdrs = ["trpc_telemetry.h"],
    deps = [
        ":telemetry",
        ":telemetry_factory",
        "//trpc/client:make_client_context",
        "//trpc/tracing:tracing_filter_index",
    ],
)

cc_test(
    name = "telemetry_factory_test",
    srcs = ["telemetry_factory_test.cc"],
    deps = [
        ":telemetry_factory",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trpc_telemetry_test",
    srcs = ["trpc_telemetry_test.cc"],
    deps = [
        ":trpc_telemetry",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
