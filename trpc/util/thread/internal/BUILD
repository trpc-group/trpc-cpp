licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "memory_barrier",
    srcs = ["memory_barrier.cc"],
    hdrs = ["memory_barrier.h"],
    deps = [
        "//trpc/util:check",
        "//trpc/util/internal:never_destroyed",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "object_array",
    hdrs = ["object_array.h"],
    deps = [
        "//trpc/util:align",
        "//trpc/util:function",
        "//trpc/util:likely",
        "//trpc/util/internal:never_destroyed",
        "//trpc/util/log:logging",
    ],
)
