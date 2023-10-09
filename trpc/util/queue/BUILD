licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "bounded_mpmc_queue",
    hdrs = ["bounded_mpmc_queue.h"],
    deps = [
        "//trpc/util/algorithm:power_of_two",
        "//trpc/util/queue/detail:util",
    ],
)

cc_test(
    name = "bounded_mpmc_queue_test",
    srcs = ["bounded_mpmc_queue_test.cc"],
    deps = [
        ":bounded_mpmc_queue",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "bounded_mpsc_queue",
    hdrs = ["bounded_mpsc_queue.h"],
    deps = [
        "//trpc/util/algorithm:power_of_two",
        "//trpc/util/queue/detail:util",
    ],
)

cc_test(
    name = "bounded_mpsc_queue_test",
    srcs = ["bounded_mpsc_queue_test.cc"],
    deps = [
        ":bounded_mpsc_queue",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "bounded_spmc_queue",
    hdrs = ["bounded_spmc_queue.h"],
    deps = [
        "//trpc/util/algorithm:power_of_two",
        "//trpc/util/queue/detail:util",
    ],
)

cc_test(
    name = "bounded_spmc_queue_test",
    srcs = ["bounded_spmc_queue_test.cc"],
    deps = [
        ":bounded_spmc_queue",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "lockfree_queue",
    hdrs = ["lockfree_queue.h"],
)

cc_test(
    name = "lockfree_queue_test",
    srcs = ["lockfree_queue_test.cc"],
    deps = [
        ":lockfree_queue",
        "//trpc/util/algorithm:power_of_two",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "unbounded_spmc_queue",
    hdrs = ["unbounded_spmc_queue.h"],
)
