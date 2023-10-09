licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "task_executor",
    hdrs = ["task_executor.h"],
    deps = [
        "//trpc/common:status",
    ],
)

cc_library(
    name = "task_executer",
    hdrs = ["task_executer.h"],
    deps = [
        ":task_executor",
    ],
)
