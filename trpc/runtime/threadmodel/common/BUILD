licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "task_type",
    hdrs = ["task_type.h"],
)

cc_library(
    name = "msg_task",
    hdrs = ["msg_task.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":task_type",
        "//trpc/util:function",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "timer_task",
    hdrs = ["timer_task.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/util:function",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "worker_thread",
    srcs = ["worker_thread.cc"],
    hdrs = ["worker_thread.h"],
    deps = [
        "//trpc/util/thread:thread_helper",
    ],
)
