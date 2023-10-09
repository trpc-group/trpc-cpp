licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "bounded_queue",
    hdrs = ["bounded_queue.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/util:likely",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "bounded_queue_test",
    srcs = ["bounded_queue_test.cc"],
    deps = [
        ":bounded_queue",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "fixed_arena_allocator",
    hdrs = ["fixed_arena_allocator.h"],
    deps = [],
)

cc_library(
    name = "container",
    hdrs = ["container.h"],
    deps = [
        ":fixed_arena_allocator",
    ],
)
