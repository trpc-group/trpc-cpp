licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "overload_control_testing",
    hdrs = ["overload_control_testing.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/codec:protocol",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/filter:filter_manager",
        "//trpc/overload_control:server_overload_controller",
        "//trpc/server:service",
        "//trpc/server/testing:service_adapter_testing",
        "@com_google_googletest//:gtest_main",
    ],
)
