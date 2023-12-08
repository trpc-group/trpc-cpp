licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "bounded_mpmc_queue",
    hdrs = ["bounded_mpmc_queue.h"],
    deps = [
        "//trpc/util/queue:bounded_mpmc_queue",
    ],
)

cc_library(
    name = "bounded_mpsc_queue",
    hdrs = ["bounded_mpsc_queue.h"],
    deps = [
        "//trpc/util/queue:bounded_mpsc_queue",
    ],
)

cc_library(
    name = "compile",
    hdrs = ["compile.h"],
)

cc_library(
    name = "cpu",
    srcs = ["cpu.cc"],
    hdrs = ["cpu.h"],
    deps = [
        "//trpc/util:check",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
        "//trpc/util/string:string_helper",
        "//trpc/util/thread:thread_helper",
    ],
)

cc_library(
    name = "futex_notifier",
    hdrs = ["futex_notifier.h"],
    deps = [
        "//trpc/util/chrono",
    ],
)

cc_test(
    name = "futex_notifier_test",
    srcs = ["futex_notifier_test.cc"],
    deps = [
        ":futex_notifier",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "latch",
    srcs = ["latch.cc"],
    hdrs = ["latch.h"],
    deps = [
        "//trpc/util:check",
    ],
)

cc_test(
    name = "latch_test",
    srcs = ["latch_test.cc"],
    deps = [
        ":latch",
        "//trpc/util/chrono",
        "//trpc/util/log:logging",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "memory_barrier",
    hdrs = ["memory_barrier.h"],
    deps = [
        "//trpc/util/thread/internal:memory_barrier",
    ],
)

cc_library(
    name = "predicate_notifier",
    hdrs = ["predicate_notifier.h"],
)

cc_library(
    name = "process_info",
    srcs = ["process_info.cc"],
    hdrs = ["process_info.h"],
    deps = [
        "//trpc/util/string:string_util",
    ],
)

cc_test(
    name = "process_info_test",
    srcs = ["process_info_test.cc"],
    deps = [
        ":process_info",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "spinlock",
    srcs = ["spinlock.cc"],
    hdrs = ["spinlock.h"],
    deps = [
        ":compile",
        "//trpc/util:likely",
    ],
)

cc_test(
    name = "spinlock_test",
    srcs = ["spinlock_test.cc"],
    deps = [
        ":latch",
        ":spinlock",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "thread_cond",
    hdrs = ["thread_cond.h"],
    deps = [
    ],
)

cc_library(
    name = "thread_helper",
    srcs = ["thread_helper.cc"],
    hdrs = ["thread_helper.h"],
    linkopts = [
        "-ldl",
        "-pthread",
    ],
    deps = [
        "//trpc/util:check",
        "//trpc/util:string_helper",
        "//trpc/util:string_util",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "thread_helper_test",
    srcs = ["thread_helper_test.cc"],
    deps = [
        ":cpu",
        ":thread_helper",
        "//trpc/util/algorithm:random",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "thread_local",
    hdrs = [
        "thread_local.h",
    ],
    deps = [
        "//trpc/util:function",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
        "//trpc/util/thread/internal:object_array",
    ],
)

cc_test(
    name = "thread_local_test",
    srcs = ["thread_local_test.cc"],
    deps = [
        ":thread_local",
        "//trpc/util:latch",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "thread_lock",
    hdrs = [
        "thread_lock.h",
    ],
)

cc_library(
    name = "thread_monitor",
    hdrs = ["thread_monitor.h"],
    deps = [
        ":thread_cond",
        ":thread_mutex",
    ],
)

cc_test(
    name = "thread_monitor_test",
    srcs = ["thread_monitor_test.cc"],
    linkopts = ["-pthread"],
    deps = [
        ":thread_monitor",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "thread_mutex",
    hdrs = [
        "thread_mutex.h",
    ],
    deps = [
        ":thread_cond",
        ":thread_lock",
    ],
)

cc_library(
    name = "thread_pool_option",
    hdrs = ["thread_pool_option.h"],
    deps = [],
)

cc_library(
    name = "sq_thread_pool",
    srcs = ["sq_thread_pool.cc"],
    hdrs = ["sq_thread_pool.h"],
    deps = [
        ":bounded_mpmc_queue",
        ":futex_notifier",
        ":thread_pool_option",
        "//trpc/util:bind_core_manager",
        "//trpc/util:function",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "sq_thread_pool_test",
    srcs = ["sq_thread_pool_test.cc"],
    deps = [
        ":sq_thread_pool",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mq_thread_pool",
    srcs = ["mq_thread_pool.cc"],
    hdrs = [
        "mq_thread_pool.h",
        "mq_thread_pool_task.h",
        "mq_thread_pool_worker.h",
        "predicate_notifier.h",
    ],
    deps = [
        ":bounded_mpmc_queue",
        ":thread_pool_option",
        ":unbounded_spmc_queue",
        "//trpc/util:bind_core_manager",
        "//trpc/util:function",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "mq_thread_pool_test",
    srcs = ["mq_thread_pool_test.cc"],
    deps = [
        ":mq_thread_pool",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "unbounded_spmc_queue",
    hdrs = ["unbounded_spmc_queue.h"],
    deps = [
        "//trpc/util/queue:unbounded_spmc_queue",
    ],
)

cc_library(
    name = "thread_pool",
    hdrs = ["thread_pool.h"],
    deps = [
        ":mq_thread_pool",
        ":sq_thread_pool",
        "//trpc/util:function",
    ],
)

cc_test(
    name = "thread_pool_test",
    srcs = ["thread_pool_test.cc"],
    deps = [
        ":thread_pool",
        "@com_google_googletest//:gtest_main",
    ],
)
