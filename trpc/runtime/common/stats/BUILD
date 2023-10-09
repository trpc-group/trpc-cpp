licenses(["notice"])

package(default_visibility = ["//visibility:public"])


cc_library(
    name = "frame_stats_testing",
    hdrs = ["frame_stats_testing.h"],
    deps = [
        "//trpc/common:trpc_plugin",
        "//trpc/common/config:trpc_config",
        "//trpc/metrics",
        "//trpc/metrics:metrics_factory",
        "@com_google_googletest//:gtest_main",
    ],
)


cc_library(
    name = "backup_request_stats",
    srcs = ["backup_request_stats.cc"],
    hdrs = ["backup_request_stats.h"],
    deps = [
        "//trpc/metrics:metrics_factory",
        "//trpc/tvar/basic_ops:reducer",
        "//trpc/util/log:logging",
    ],
)


cc_test(
    name = "backup_request_stats_test",
    srcs = ["backup_request_stats_test.cc"],
    data = [
        "server_stats.yaml",
    ],
    deps = [
        ":backup_request_stats",
        ":frame_stats_testing",
        "@com_google_googletest//:gtest_main",
    ],
)


cc_library(
    name = "server_stats",
    srcs = ["server_stats.cc"],
    hdrs = ["server_stats.h"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/tvar/basic_ops:reducer",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "server_stats_test",
    srcs = ["server_stats_test.cc"],
    data = [
        "server_stats.yaml",
    ],
    deps = [
        ":frame_stats_testing",
        ":server_stats",
    ],
)

cc_library(
    name = "frame_stats",
    srcs = ["frame_stats.cc"],
    hdrs = ["frame_stats.h"],
    deps = [
        ":backup_request_stats",
        ":server_stats",
        "//trpc/common/config:trpc_config",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/tvar/basic_ops:reducer",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "frame_stats_test",
    srcs = ["frame_stats_test.cc"],
    data = [
        "server_stats.yaml",
    ],
    deps = [
        ":frame_stats",
        ":frame_stats_testing",
    ],
)
