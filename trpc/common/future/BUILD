# Description: trpc-cpp.
# Deprecated, use trpc/future instead.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "function_traits",
    hdrs = ["function_traits.h"],
    deps = [
        "//trpc/future:function_traits",
    ],
)

cc_library(
    name = "basics",
    hdrs = [
        "basics.h",
        "types.h",
    ],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/future:basics",
    ],
)

cc_library(
    name = "exception",
    hdrs = ["exception.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/future:exception",
    ],
)

cc_library(
    name = "executor",
    hdrs = ["executor.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/future:executor",
    ],
)

cc_library(
    name = "reactor_executor",
    hdrs = ["reactor_executor.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/future:reactor_executor",
    ],
)

cc_library(
    name = "future",
    hdrs = ["future.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/future",
    ],
)

cc_library(
    name = "future_utility",
    hdrs = ["future_utility.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/future:future_utility",
    ],
)
