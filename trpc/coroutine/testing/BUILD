licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fiber_runtime_test",
    hdrs = ["fiber_runtime.h"],
    deps = [
        "//trpc/coroutine:fiber",
        "//trpc/coroutine/fiber:runtime",
        "//trpc/runtime:runtime",
        "//trpc/runtime/threadmodel/fiber/detail:fiber_impl",
        "//trpc/util/thread:latch",
    ],
)

cc_library(
    name = "mock_fiber",
    hdrs = ["mock_fiber.h"],
    deps = [
        "//trpc/util:function",
    ],
)

cc_test(
    name = "fiber_runtime_testing",
    srcs = ["fiber_runtime_testing.cc"],
    deps = [
        ":fiber_runtime_test",
        "//trpc/coroutine:fiber",
        "//trpc/util:time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
