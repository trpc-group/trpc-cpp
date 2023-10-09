licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "runtime_info_reporter",
    srcs = ["runtime_info_reporter.cc"],
    hdrs = ["runtime_info_reporter.h"],
    deps = [
        "//trpc/admin:base_funcs",
        "//trpc/common/config:config_helper",
        "//trpc/metrics:metrics_factory",
        "//trpc/metrics:trpc_metrics_report",
        "//trpc/runtime",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/runtime/common/heartbeat:heartbeat_report",
        "//trpc/runtime/common/stats:frame_stats",
        "//trpc/stream:stream_var",
        "//trpc/util/thread:process_info",
    ],
)

cc_test(
    name = "runtime_info_reporter_test",
    srcs = ["runtime_info_reporter_test.cc"],
    data = ["//trpc/runtime/common/runtime_info_report:test.yaml"],
    deps = [
        ":runtime_info_reporter",
        "//trpc/metrics:trpc_metrics",
        "//trpc/runtime/common:periphery_task_scheduler",
        "@com_google_googletest//:gtest_main",
    ],
)
