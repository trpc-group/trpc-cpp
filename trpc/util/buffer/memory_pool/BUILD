# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common",
    srcs = ["common.cc"],
    hdrs = ["common.h"],
    deps = [
        "//trpc/util/algorithm:power_of_two",
    ],
)

cc_test(
    name = "common_test",
    srcs = ["common_test.cc"],
    deps = [
        ":common",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "disabled_memory_pool",
    srcs = ["disabled_memory_pool.cc"],
    hdrs = ["disabled_memory_pool.h"],
    deps = [
        ":common",
        "//trpc/util:likely",
    ],
)

cc_test(
    name = "disabled_memory_pool_test",
    srcs = ["disabled_memory_pool_test.cc"],
    deps = [
        ":disabled_memory_pool",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "global_memory_pool",
    srcs = ["global_memory_pool.cc"],
    hdrs = ["global_memory_pool.h"],
    deps = [
        ":common",
        "//trpc/util:check",
        "//trpc/util:likely",
        "//trpc/util/internal:never_destroyed",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "global_memory_pool_test",
    srcs = ["global_memory_pool_test.cc"],
    deps = [
        ":global_memory_pool",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "memory_pool",
    srcs = ["memory_pool.cc"],
    hdrs = ["memory_pool.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_mem_pool": ["TRPC_DISABLED_MEM_POOL"],
                  "//trpc:trpc_shared_nothing_mem_pool": ["TRPC_SHARED_NOTHING_MEM_POOL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":common",
        ":disabled_memory_pool",
        ":global_memory_pool",
        ":shared_nothing_memory_pool",
        "//trpc/util:check",
        "//trpc/util:ref_ptr",
    ],
)

cc_test(
    name = "memory_pool_test",
    srcs = ["memory_pool_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_shared_nothing_mem_pool": ["TRPC_SHARED_NOTHING_MEM_POOL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":memory_pool",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "shared_nothing_memory_pool",
    srcs = ["shared_nothing_memory_pool.cc"],
    hdrs = ["shared_nothing_memory_pool.h"],
    deps = [
        ":common",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "shared_nothing_memory_pool_test",
    srcs = ["shared_nothing_memory_pool_test.cc"],
    deps = [
        ":shared_nothing_memory_pool",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
