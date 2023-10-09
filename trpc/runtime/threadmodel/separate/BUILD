licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "separate_thread_model",
    srcs = ["separate_thread_model.cc"],
    hdrs = ["separate_thread_model.h"],
    deps = [
        ":handle_worker_thread",
        ":io_worker_thread",
        ":separate_scheduling",
        "//trpc/runtime/threadmodel:thread_model",
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/runtime/threadmodel/common:timer_task",
        "//trpc/util:random",
        "//trpc/util/log:logging",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "handle_worker_thread",
    srcs = ["handle_worker_thread.cc"],
    hdrs = ["handle_worker_thread.h"],
    deps = [
        ":separate_scheduling",
        "//trpc/runtime/threadmodel/common:worker_thread",
        "//trpc/util/thread:latch",
        "//trpc/util/thread:thread_helper",
    ],
)

cc_library(
    name = "separate_scheduling",
    srcs = ["separate_scheduling.cc"],
    hdrs = ["separate_scheduling.h"],
    deps = [
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/runtime/threadmodel/common:timer_task",
        "//trpc/util:function",
    ],
)

cc_library(
    name = "io_worker_thread",
    srcs = ["io_worker_thread.cc"],
    hdrs = ["io_worker_thread.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/default:reactor_impl",
        "//trpc/runtime/threadmodel/common:worker_thread",
        "//trpc/util/thread:latch",
        "//trpc/util/thread:thread_helper",
    ],
)

cc_test(
    name = "separate_thread_model_test",
    srcs = ["separate_thread_model_test.cc"],
    deps = [
        ":separate_thread_model",
        "//trpc/runtime/iomodel/reactor/default:reactor_impl",
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/runtime/threadmodel/separate/non_steal:non_steal_scheduling",
        "//trpc/runtime/threadmodel/separate/steal:steal_scheduling",
        "//trpc/util/object_pool:object_pool_ptr",
        "//trpc/util/thread:thread_helper",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
