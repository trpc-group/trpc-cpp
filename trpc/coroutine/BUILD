licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "async",
    hdrs = ["async.h"],
    deps = [
        ":fiber_basic",
        "//trpc/common/future",
        "//trpc/common/future:basics",
    ],
)

cc_library(
    name = "fiber",
    hdrs = [
        "fiber.h",
        "fiber_blocking_bounded_queue.h",
        "fiber_blocking_noncontiguous_buffer.h",
        "fiber_condition_variable.h",
        "fiber_event.h",
        "fiber_execution_context.h",
        "fiber_latch.h",
        "fiber_local.h",
        "fiber_mutex.h",
        "fiber_timed_mutex.h",
        "fiber_timer.h",
    ],
    deps = [
        ":fiber_basic",
        ":fiber_timer",
        "//trpc/util:function",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_library(
    name = "fiber_timer",
    srcs = ["fiber_timer.cc"],
    hdrs = ["fiber_timer.h"],
    deps = [
        ":fiber_basic",
        "//trpc/runtime/threadmodel/fiber/detail:fiber_impl",
    ],
)

cc_library(
    name = "future",
    hdrs = ["future.h"],
    deps = [
        "//trpc/common/future",
        "//trpc/runtime/threadmodel/fiber/detail:fiber_impl",
        "//trpc/util:delayed_init",
    ],
)

cc_library(
    name = "fiber_local",
    hdrs = ["fiber_local.h"],
    deps = [
        "//trpc/runtime/threadmodel/fiber/detail:fiber_impl",
        "//trpc/util/internal:index_alloc",
    ],
)

cc_library(
    name = "fiber_seqlock",
    srcs = ["fiber_seqlock.cc"],
    hdrs = [
        "fiber_mutex.h",
        "fiber_seqlock.h",
    ],
    deps = [
        "//trpc/runtime/threadmodel/fiber/detail:fiber_impl",
        "//trpc/util:likely",
        "//trpc/util/thread:compile",
        "//trpc/util/thread:memory_barrier",
    ],
)

cc_library(
    name = "fiber_shared_mutex",
    srcs = ["fiber_shared_mutex.cc"],
    hdrs = [
        "fiber_condition_variable.h",
        "fiber_mutex.h",
        "fiber_shared_mutex.h",
    ],
    deps = [
        ":fiber",
        "//trpc/util:likely",
    ],
)

#############################################
# TARGET BELOW ARE FOR INTERNAL USE.        #
#                                           #
# YOU SHOULD USE `fiber` ABOVE INSTEAD.     #
#############################################

cc_library(
    name = "fiber_basic",
    srcs = [
        "fiber.cc",
        "fiber_condition_variable.cc",
        "fiber_execution_context.cc",
        "fiber_latch.cc",
        "fiber_timed_mutex.cc",
        "fiber_timer.cc",
    ],
    hdrs = [
        "fiber.h",
        "fiber_condition_variable.h",
        "fiber_event.h",
        "fiber_execution_context.h",
        "fiber_latch.h",
        "fiber_mutex.h",
        "fiber_timed_mutex.h",
        "fiber_timer.h",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":fiber_local",
        "//trpc/coroutine/fiber:runtime",
        "//trpc/log:trpc_log",
        "//trpc/runtime/threadmodel/fiber/detail:fiber_impl",
        "//trpc/util:check",
        "//trpc/util:likely",
        "//trpc/util:random",
        "//trpc/util:string_helper",
        "//trpc/util/object_pool:object_pool_ptr",
        "//trpc/util/thread:cpu",
        "//trpc/util/thread:thread_helper",
    ],
)

cc_test(
    name = "fiber_seqlock_test",
    srcs = ["fiber_seqlock_test.cc"],
    deps = [
        ":fiber",
        ":fiber_seqlock",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/util:latch",
        "//trpc/util:likely",
        "//trpc/util:random",
        "//trpc/util:time",
        "//trpc/util/chrono",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_mutex_test",
    srcs = ["fiber_mutex_test.cc"],
    deps = [
        ":fiber",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_shared_mutex_test",
    srcs = ["fiber_shared_mutex_test.cc"],
    deps = [
        ":fiber",
        ":fiber_shared_mutex",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/util:latch",
        "//trpc/util:random",
        "//trpc/util:time",
        "//trpc/util/chrono",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_timed_mutex_test",
    srcs = ["fiber_timed_mutex_test.cc"],
    deps = [
        ":fiber",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_condition_variable_test",
    srcs = ["fiber_condition_variable_test.cc"],
    deps = [
        ":fiber",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_event_test",
    srcs = ["fiber_event_test.cc"],
    deps = [
        ":fiber",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_latch_test",
    srcs = ["fiber_latch_test.cc"],
    deps = [
        ":fiber",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_test",
    srcs = ["fiber_test.cc"],
    deps = [
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/util:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_local_test",
    srcs = ["fiber_local_test.cc"],
    deps = [
        ":fiber",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_execution_context_test",
    srcs = ["fiber_execution_context_test.cc"],
    deps = [
        ":fiber",
        ":fiber_basic",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "future_test",
    srcs = ["future_test.cc"],
    deps = [
        ":future",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine/fiber:runtime",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/util:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "async_test",
    srcs = ["async_test.cc"],
    deps = [
        ":async",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine:future",
        "//trpc/coroutine/fiber:runtime",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/util:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_blocking_bounded_queue_test",
    srcs = ["fiber_blocking_bounded_queue_test.cc"],
    deps = [
        ":future",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine/fiber:runtime",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/util:time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_blocking_noncontiguous_buffer_test",
    srcs = ["fiber_blocking_noncontiguous_buffer_test.cc"],
    deps = [
        ":future",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine/fiber:runtime",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/util:time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
