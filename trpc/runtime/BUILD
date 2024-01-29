licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "runtime_state",
    hdrs = ["runtime_state.h"],
)

cc_library(
    name = "fiber_runtime",
    srcs = ["fiber_runtime.cc"],
    hdrs = ["fiber_runtime.h"],
    deps = [
        ":runtime_state",
        "//trpc/common/config:trpc_config",
        "//trpc/runtime/threadmodel:thread_model",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/runtime/threadmodel/fiber:fiber_thread_model",
        "//trpc/util:random",
        "//trpc/util/log:logging",
        "//trpc/util/thread:thread_helper",
    ],
)

cc_library(
    name = "merge_runtime",
    srcs = ["merge_runtime.cc"],
    hdrs = ["merge_runtime.h"],
    deps = [
        ":runtime_state",
        "//trpc/common/config:trpc_config",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/threadmodel:thread_model",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/runtime/threadmodel/common:timer_task",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/util:check",
        "//trpc/util/log:logging",
        "//trpc/util/thread:thread_helper",
    ],
)

cc_library(
    name = "separate_runtime",
    srcs = ["separate_runtime.cc"],
    hdrs = ["separate_runtime.h"],
    deps = [
        ":runtime_state",
        "//trpc/common/config:trpc_config",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/threadmodel:thread_model",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/runtime/threadmodel/common:timer_task",
        "//trpc/runtime/threadmodel/separate:separate_thread_model",
        "//trpc/runtime/threadmodel/separate/non_steal:non_steal_scheduling",
        "//trpc/runtime/threadmodel/separate/steal:steal_scheduling",
        "//trpc/util:check",
        "//trpc/util/log:logging",
        "//trpc/util/thread:thread_helper",
    ],
)

cc_library(
    name = "runtime",
    srcs = ["runtime.cc"],
    hdrs = ["runtime.h"],
    deps = [
        ":fiber_runtime",
        ":merge_runtime",
        ":runtime_compatible",
        ":separate_runtime",
        "//trpc/common/config:trpc_config",
        "//trpc/coroutine:fiber",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_reactor",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/util:latch",
        "//trpc/util:net_util",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/buffer/memory_pool:common",
        "//trpc/util/internal:time_keeper",
    ],
)

cc_library(
    name = "runtime_compatible",
    srcs = ["runtime_compatible.cc"],
    hdrs = ["runtime_compatible.h"],
    deps = [
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/runtime/threadmodel/separate:separate_thread_model",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "init_runtime",
    hdrs = ["init_runtime.h"],
    deps = [
        ":runtime",
    ],
)
