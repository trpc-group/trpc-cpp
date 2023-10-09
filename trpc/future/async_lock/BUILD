# Description: async_lock.

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "async_lock",
    hdrs = [
        "rwlock.h",
        "semaphore.h",
        "with_lock.h",
    ],
    deps = [
        "//trpc/future",
        "//trpc/runtime/threadmodel/common:worker_thread",
        "//trpc/util/internal:singly_linked_list",
    ],
)

cc_test(
    name = "semaphore_test",
    srcs = ["semaphore_test.cc"],
    deps = [
        ":async_lock",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "rwlock_test",
    srcs = ["rwlock_test.cc"],
    deps = [
        ":async_lock",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "with_lock_test",
    srcs = ["with_lock_test.cc"],
    data = [
        "//trpc/runtime/threadmodel/testing:merge.yaml",
    ],
    deps = [
        ":async_lock",
        "//trpc/common:async_timer",
        "//trpc/runtime:merge_runtime",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
