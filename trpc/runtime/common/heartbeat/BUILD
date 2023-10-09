licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "heartbeat_info",
    srcs = ["heartbeat_info.cc"],
    hdrs = ["heartbeat_info.h"],
    deps = [
        "//trpc/runtime/threadmodel/common:worker_thread",
        "//trpc/util:function",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "heartbeat_report",
    srcs = ["heartbeat_report.cc"],
    hdrs = ["heartbeat_report.h"],
    deps = [
        ":heartbeat_info",
        "//trpc/common/config:trpc_config",
        "//trpc/naming:trpc_naming",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/runtime/threadmodel/common:worker_thread",
        "//trpc/util:time",
        "//trpc/util/log:logging",
        "//trpc/util/queue:bounded_mpmc_queue",
        "@com_github_fmtlib_fmt//:fmtlib",
    ],
)

cc_test(
    name = "heartbeat_report_test",
    srcs = ["heartbeat_report_test.cc"],
    data = [
        "//trpc/runtime/testing:heartbeat_report_test.yaml",
    ],
    deps = [
        ":heartbeat_report",
        "//trpc/common/config:trpc_config",
        "//trpc/metrics",
        "//trpc/metrics:metrics_factory",
        "//trpc/metrics:trpc_metrics",
        "//trpc/runtime/common/runtime_info_report:runtime_info_reporter",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
