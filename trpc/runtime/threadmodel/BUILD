licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "thread_model",
    hdrs = ["thread_model.h"],
    deps = [
        "//trpc/runtime/threadmodel/common:msg_task",
    ],
)

cc_library(
    name = "thread_model_manager",
    srcs = ["thread_model_manager.cc"],
    hdrs = ["thread_model_manager.h"],
    deps = [
        ":thread_model",
        "//trpc/util/log:logging",
    ],
)
