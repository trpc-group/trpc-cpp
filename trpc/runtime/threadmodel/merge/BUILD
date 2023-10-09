licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "merge_thread_model",
    srcs = ["merge_thread_model.cc"],
    hdrs = ["merge_thread_model.h"],
    deps = [
        ":merge_worker_thread",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/threadmodel:thread_model",
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/runtime/threadmodel/common:timer_task",
        "//trpc/util:likely",
        "//trpc/util:random",
        "//trpc/util/log:logging",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "merge_worker_thread",
    srcs = ["merge_worker_thread.cc"],
    hdrs = ["merge_worker_thread.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/default:reactor_impl",
        "//trpc/runtime/threadmodel/common:worker_thread",
        "//trpc/util/thread:latch",
        "//trpc/util/thread:thread_helper",
    ],
)

cc_test(
    name = "merge_thread_model_test",
    srcs = ["merge_thread_model_test.cc"],
    deps = [
        ":merge_thread_model",
        "//trpc/runtime/iomodel/reactor/default:reactor_impl",
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/util/object_pool:object_pool_ptr",
        "//trpc/util/thread:thread_helper",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
