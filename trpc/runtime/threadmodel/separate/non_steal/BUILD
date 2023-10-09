licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "non_steal_scheduling",
    srcs = ["non_steal_scheduling.cc"],
    hdrs = ["non_steal_scheduling.h"],
    deps = [
        "//trpc/runtime/common/heartbeat:heartbeat_info",
        "//trpc/runtime/iomodel/reactor/default:timer_queue",
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/runtime/threadmodel/common:timer_task",
        "//trpc/runtime/threadmodel/separate:separate_scheduling",
        "//trpc/util:time",
        "//trpc/util/log:logging",
        "//trpc/util/object_pool:object_pool_ptr",
        "//trpc/util/queue:bounded_mpmc_queue",
        "//trpc/util/queue:bounded_mpsc_queue",
        "//trpc/util/thread:thread_monitor",
    ],
)
