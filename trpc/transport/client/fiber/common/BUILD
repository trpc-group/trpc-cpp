licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "call_context",
    hdrs = ["call_context.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/coroutine:fiber_basic",
        "//trpc/transport/client:client_transport",
        "//trpc/transport/client:retry_info_def",
        "//trpc/util:align",
        "//trpc/util:function",
        "//trpc/util/object_pool:object_pool_ptr",
        "//trpc/util/thread:spinlock",
    ],
)

cc_library(
    name = "fiber_backup_request_retry",
    srcs = ["fiber_backup_request_retry.cc"],
    hdrs = ["fiber_backup_request_retry.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/coroutine:fiber",
        "//trpc/transport/client:retry_info_def",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "fiber_client_connection_handler",
    srcs = ["fiber_client_connection_handler.cc"],
    hdrs = ["fiber_client_connection_handler.h"],
    deps = [
        "//trpc/filter:filter_point",
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/stream:stream_handler",
        "//trpc/transport/client:trans_info",
    ],
)

cc_library(
    name = "fiber_client_connection_handler_factory",
    srcs = ["fiber_client_connection_handler_factory.cc"],
    hdrs = ["fiber_client_connection_handler_factory.h"],
    deps = [
        ":fiber_client_connection_handler",
    ],
)

cc_library(
    name = "sharded_call_map",
    hdrs = ["sharded_call_map.h"],
    deps = [
        ":call_context",
        "//trpc/coroutine:fiber_basic",
        "//trpc/util:align",
        "//trpc/util:hash_util",
        "//trpc/util:likely",
    ],
)
