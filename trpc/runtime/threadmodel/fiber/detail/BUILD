licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "context",
    srcs = select({
        "@bazel_tools//src/conditions:linux_x86_64": glob(["fcontext/x86_64/*.S"]),
        "@bazel_tools//src/conditions:linux_aarch64": glob(["fcontext/aarch64/*.S"]),
    }),
)

cc_library(
    name = "assembly",
    hdrs = ["assembly.h"],
    deps = [
        "//trpc/log:trpc_log",
        "//trpc/util:check",
    ],
)

cc_library(
    name = "stack_allocator_impl",
    srcs = ["stack_allocator_impl.cc"],
    hdrs = ["stack_allocator_impl.h"],
    deps = [
        ":assembly",
        "//trpc/util:check",
        "//trpc/util:deferred",
        "//trpc/util:likely",
        "//trpc/util/internal:never_destroyed",
    ],
)

cc_library(
    name = "fiber_impl",
    srcs = [
        "fiber_desc.cc",
        "fiber_entity.cc",
        "fiber_worker.cc",
        "scheduling/scheduling.cc",
        "scheduling/v1/run_queue.cc",
        "scheduling/v1/scheduling_impl.cc",
        "scheduling/v2/local_queue.cc",
        "scheduling/v2/scheduling_impl.cc",
        "scheduling_group.cc",
        "timer_worker.cc",
        "waitable.cc",
    ],
    hdrs = [
        "fiber_desc.h",
        "fiber_entity.h",
        "fiber_id_gen.h",
        "fiber_worker.h",
        "runnable_entity.h",
        "scheduling/scheduling.h",
        "scheduling/scheduling_var.h",
        "scheduling/v1/run_queue.h",
        "scheduling/v1/scheduling_impl.h",
        "scheduling/v2/local_queue.h",
        "scheduling/v2/scheduling_impl.h",
        "scheduling_group.h",
        "timer_worker.h",
        "waitable.h",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "stack_allocator_impl",
        ":assembly",
        ":context",
        "//trpc/log:trpc_log",
        "//trpc/runtime/threadmodel/common:worker_thread",
        "//trpc/tvar/compound_ops:internal_latency",
        "//trpc/util:align",
        "//trpc/util:check",
        "//trpc/util:deferred",
        "//trpc/util:delayed_init",
        "//trpc/util:doubly_linked_list",
        "//trpc/util:erased_ptr",
        "//trpc/util:latch",
        "//trpc/util:likely",
        "//trpc/util:random",
        "//trpc/util:ref_ptr",
        "//trpc/util:string_helper",
        "//trpc/util:unique_id",
        "//trpc/util/chrono",
        "//trpc/util/chrono:tsc",
        "//trpc/util/internal:casting",
        "//trpc/util/object_pool:object_pool_ptr",
        "//trpc/util/queue:bounded_mpmc_queue",
        "//trpc/util/queue:bounded_spmc_queue",
        "//trpc/util/thread:futex_notifier",
        "//trpc/util/thread:latch",
        "//trpc/util/thread:predicate_notifier",
        "//trpc/util/thread:spinlock",
        "//trpc/util/thread:thread_helper",
        "@com_github_gflags_gflags//:gflags",
    ],
)

cc_test(
    name = "fiber_entity_test",
    srcs = ["fiber_entity_test.cc"],
    deps = [
        ":fiber_impl",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_worker_test",
    srcs = ["fiber_worker_test.cc"],
    deps = [
        ":fiber_impl",
        ":testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "timer_worker_test",
    srcs = ["timer_worker_test.cc"],
    deps = [
        ":fiber_impl",
        "//trpc/util:latch",
        "//trpc/util:random",
        "//trpc/util/chrono",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "stack_allocator_impl_test",
    srcs = ["stack_allocator_impl_test.cc"],
    deps = [
        ":stack_allocator_impl",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "testing",
    hdrs = ["testing.h"],
    deps = [
        ":fiber_impl",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine/testing:fiber_runtime_test",
    ],
)

cc_test(
    name = "scheduling_group_test",
    srcs = ["scheduling_group_test.cc"],
    deps = [
        ":fiber_impl",
        ":testing",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "waitable_test",
    srcs = ["waitable_test.cc"],
    deps = [
        ":fiber_impl",
        ":testing",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "run_queue_test",
    srcs = ["scheduling/v1/run_queue_test.cc"],
    deps = [
        ":fiber_impl",
        "//trpc/util:latch",
        "//trpc/util:random",
        "@com_google_googletest//:gtest_main",
    ],
)
