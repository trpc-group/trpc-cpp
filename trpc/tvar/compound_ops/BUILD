# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "window",
    hdrs = ["window.h"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/tvar/common:tvar_group",
        "//trpc/tvar/common:write_mostly",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "window_test",
    srcs = ["window_test.cc"],
    data = ["//trpc/tvar/testing:series.yaml"],
    deps = [
        ":window",
        "//trpc/common/config:trpc_config",
        "//trpc/tvar/basic_ops:passive_status",
        "//trpc/tvar/basic_ops:recorder",
        "//trpc/tvar/basic_ops:reducer",
        "//trpc/tvar/common:op_util",
        "//trpc/tvar/common:tvar_group",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "latency_recorder",
    srcs = ["latency_recorder.cc"],
    hdrs = ["latency_recorder.h"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/tvar/basic_ops:passive_status",
        "//trpc/tvar/basic_ops:recorder",
        "//trpc/tvar/basic_ops:reducer",
        "//trpc/tvar/common:percentile",
        "//trpc/tvar/common:series",
        "//trpc/tvar/common:tvar_group",
        "//trpc/tvar/common:write_mostly",
        "//trpc/tvar/compound_ops:window",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "latency_recorder_test",
    srcs = ["latency_recorder_test.cc"],
    data = ["//trpc/tvar/testing:series.yaml"],
    deps = [
        ":latency_recorder",
        "//trpc/tvar/common:tvar_group",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "internal_latency",
    hdrs = ["internal_latency.h"],
    deps = [
        "//trpc/tvar/basic_ops:reducer",
        "//trpc/tvar/common:tvar_group",
        "//trpc/tvar/compound_ops:window",
        "//trpc/util/chrono:tsc",
        "@com_github_open_source_parsers_jsoncpp//:jsoncpp",
    ],
)

cc_test(
    name = "internal_latency_test",
    srcs = ["internal_latency_test.cc"],
    data = ["//trpc/tvar/testing:series.yaml"],
    deps = [
        ":internal_latency",
        "//trpc/tvar/common:tvar_group",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "noseries_test",
    srcs = ["noseries_test.cc"],
    data = ["//trpc/tvar/testing:noseries.yaml"],
    deps = [
        ":latency_recorder",
        ":window",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
