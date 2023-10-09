licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "async_timer",
    srcs = ["async_timer.cc"],
    hdrs = ["async_timer.h"],
    deps = [
        "//trpc/common/future",
        "//trpc/runtime",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/runtime/threadmodel/common:timer_task",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/runtime/threadmodel/separate:separate_thread_model",
        "//trpc/util:check",
        "//trpc/util:function",
        "//trpc/util/chrono:time",
    ],
)

cc_test(
    name = "async_timer_test",
    srcs = ["async_timer_test.cc"],
    data = [
        "//trpc/runtime/threadmodel/testing:merge.yaml",
    ],
    deps = [
        ":async_timer",
        "//trpc/common/config:trpc_config",
        "//trpc/future:future_utility",
        "//trpc/runtime:merge_runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "function_traits",
    hdrs = ["function_traits.h"],
)

cc_library(
    name = "basics",
    hdrs = [
        "basics.h",
        "types.h",
    ],
    visibility = [
        "//visibility:public",
    ],
    deps = [
    ],
)

cc_library(
    name = "executor",
    hdrs = ["executor.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/util:function",
    ],
)

cc_library(
    name = "exception",
    hdrs = ["exception.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/codec/trpc",
        "//trpc/common:status",
    ],
)

cc_test(
    name = "exception_test",
    srcs = ["exception_test.cc"],
    linkopts = ["-lpthread"],
    deps = [
        ":exception",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "reactor_executor",
    hdrs = ["reactor_executor.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":executor",
        "//trpc/runtime",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "reactor_executor_test",
    srcs = ["reactor_executor_test.cc"],
    data = [
        "//trpc/runtime/threadmodel/testing:merge.yaml",
    ],
    deps = [
        ":reactor_executor",
        "//trpc/common/config:trpc_config",
        "//trpc/common/future",
        "//trpc/runtime:merge_runtime",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "future",
    hdrs = ["future.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":exception",
        ":executor",
        ":function_traits",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/util:function",
    ],
)

cc_test(
    name = "future_test",
    srcs = ["future_test.cc"],
    linkopts = ["-lpthread"],
    deps = [
        ":future",
        "//trpc/util/thread:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "future_utility",
    hdrs = ["future_utility.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":future",
        "//trpc/util/thread:latch",
    ],
)

cc_test(
    name = "future_utility_test",
    srcs = ["future_utility_test.cc"],
    linkopts = ["-lpthread"],
    deps = [
        ":future",
        ":future_utility",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
