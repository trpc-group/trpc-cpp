licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "trpc_coroutine_task",
    hdrs = ["trpc_coroutine_task.h"],
    deps = [
        "//trpc/common:status",
    ],
)

cc_library(
    name = "trpc_task_executor",
    srcs = ["trpc_task_executor.cc"],
    hdrs = ["trpc_task_executor.h"],
    deps = [
        ":trpc_coroutine_task",
        "//trpc/common/coroutine:task_executor",
        "//trpc/coroutine:fiber",
        "//trpc/runtime",
        "//trpc/util/log:logging",
        "//trpc/util/thread:latch",
    ],
)

cc_library(
    name = "trpc_task_executer",
    hdrs = ["trpc_task_executer.h"],
    deps = [
        ":trpc_task_executor",
    ],
)

cc_test(
    name = "trpc_task_executor_test",
    srcs = ["trpc_task_executor_test.cc"],
    deps = [
        ":trpc_task_executor",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/runtime",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
