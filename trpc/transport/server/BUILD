licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "server_transport_message",
    hdrs = ["server_transport_message.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/server:server_context_h",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/object_pool",
    ],
)

cc_library(
    name = "server_transport_def",
    hdrs = ["server_transport_def.h"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":server_transport_message",
        "//trpc/codec:protocol",
        "//trpc/codec:server_codec",
        "//trpc/filter:filter_base",
        "//trpc/runtime/iomodel/reactor/common:accept_connection_info",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/runtime/iomodel/reactor/default:acceptor",
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
    name = "server_transport",
    hdrs = ["server_transport.h"],
    deps = [
        ":server_transport_def",
    ],
)
