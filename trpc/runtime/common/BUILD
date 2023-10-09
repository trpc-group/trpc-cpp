licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "periphery_task_scheduler",
    srcs = ["periphery_task_scheduler.cc"],
    hdrs = ["periphery_task_scheduler.h"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/util:function",
        "//trpc/util:likely",
        "//trpc/util:ref_ptr",
        "//trpc/util/log:logging",
        "//trpc/util/thread:latch",
        "//trpc/util/thread:thread_helper",
    ],
)

cc_test(
    name = "periphery_task_scheduler_test",
    srcs = ["periphery_task_scheduler_test.cc"],
    deps = [
        ":periphery_task_scheduler",
        "@com_google_googletest//:gtest_main",
    ],
)
