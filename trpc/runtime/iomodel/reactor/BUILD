licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "event_handler",
    srcs = ["event_handler.cc"],
    hdrs = ["event_handler.h"],
    deps = [
        "//trpc/util:ref_ptr",
    ],
)

cc_library(
    name = "poller",
    hdrs = ["poller.h"],
    deps = [
        ":event_handler",
        "//trpc/util:function",
    ],
)

cc_library(
    name = "reactor",
    hdrs = ["reactor.h"],
    deps = [
        ":event_handler",
        "//trpc/runtime/threadmodel/common:timer_task",
        "//trpc/util:function",
    ],
)
