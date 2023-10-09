licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "steal_scheduling",
    srcs = ["steal_scheduling.cc"],
    hdrs = ["steal_scheduling.h"],
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
        "//trpc/util/queue:unbounded_spmc_queue",
        "//trpc/util/thread:predicate_notifier",
    ],
)
