licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "client_transport_message",
    hdrs = ["client_transport_message.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/client:client_context",
        "//trpc/common/future",
        "//trpc/util/buffer",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "trans_info",
    hdrs = ["trans_info.h"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/codec:protocol",
        "//trpc/filter",
        "//trpc/filter:filter_point",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/util/object_pool:object_pool_ptr",
    ] + select({
        "//trpc:include_ssl": [
            "//trpc/transport/common/ssl",
        ],
        "//trpc:trpc_include_ssl": [
            "//trpc/transport/common/ssl",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "client_transport",
    hdrs = ["client_transport.h"],
    deps = [
        ":client_transport_message",
        ":preallocation_option",
        ":trans_info",
        "//trpc/future",
        "//trpc/stream",
    ],
)

cc_library(
    name = "preallocation_option",
    hdrs = ["preallocation_option.h"],
    deps = [
        "//trpc/transport/common:transport_message_common",
    ],
)

cc_library(
    name = "retry_info_def",
    hdrs = [
        "retry_info_def.h",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/transport/common:transport_message_common",
        "//trpc/util:align",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "fixed_connector_id",
    hdrs = ["fixed_connector_id.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/util/object_pool:object_pool_ptr",
        "//trpc/util/thread:spinlock",
    ],
)
