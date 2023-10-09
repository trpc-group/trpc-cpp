licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fiber_pipeline_impl",
    srcs = [
        "fiber_tcp_pipeline_connector.cc",
        "fiber_tcp_pipeline_connector_group.cc",
    ],
    hdrs = [
        "fiber_tcp_pipeline_connector.h",
        "fiber_tcp_pipeline_connector_group.h",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/coroutine:fiber_basic",
        "//trpc/runtime:fiber_runtime",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_connection",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_reactor",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_tcp_connection",
        #"//trpc/stream",
        #"//trpc/stream:fiber_stream_connection_handler",
        "//trpc/transport/client/common:client_io_handler_factory",
        "//trpc/transport/client/fiber:fiber_connector_group",
        "//trpc/transport/client/fiber/common:fiber_client_connection_handler",
        "//trpc/transport/client/fiber/common:fiber_client_connection_handler_factory",
        "//trpc/transport/client/fiber/common:sharded_call_map",
        "//trpc/util/hazptr",
        "//trpc/util/log:logging",
        "//trpc/util:align",
        "//trpc/util:likely",
        "//trpc/util:lockfree_queue",
        "//trpc/util:ref_ptr",
    ],
)
