licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fiber_thread_model",
    srcs = ["fiber_thread_model.cc"],
    hdrs = ["fiber_thread_model.h"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/runtime/threadmodel:thread_model",
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/runtime/threadmodel/fiber/detail:fiber_impl",
        "//trpc/util:deferred",
        "//trpc/util:likely",
        "//trpc/util:random",
        "//trpc/util:string_helper",
        "//trpc/util/log:logging",
        "//trpc/util/object_pool",
        "//trpc/util/thread:cpu",
        "//trpc/util/thread:thread_helper",
    ],
)
