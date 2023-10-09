licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "timingwheel_h",
    hdrs = ["timingwheel.h"],
    deps = [
        "//trpc/util:function",
        "//trpc/util:likely",
        "//trpc/util:time",
        "//trpc/util/object_pool",
    ],
)

cc_library(
    name = "timingwheel_timeout_queue",
    srcs = [],
    hdrs = ["timingwheel_timeout_queue.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/transport/client:client_transport_message",
        "//trpc/transport/client/future/common:timingwheel_h",
        "//trpc/util:function",
        "//trpc/util/container",
    ],
)

cc_test(
    name = "timingwheel_timeout_queue_test",
    srcs = ["timingwheel_timeout_queue_test.cc"],
    deps = [
        ":timingwheel_timeout_queue",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "future_connector_options",
    hdrs = ["future_connector_options.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/iomodel/reactor/common:network_address",
        "//trpc/transport/client:trans_info",
    ],
)

cc_library(
    name = "future_connector",
    hdrs = ["future_connector.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/stream:stream_provider",
        "//trpc/transport/client:client_transport_message",
        "//trpc/transport/client/future/common:future_connector_options",
    ],
)

cc_library(
    name = "future_connection_handler",
    srcs = ["future_connection_handler.cc"],
    hdrs = ["future_connection_handler.h"],
    deps = [
        "//trpc/filter:filter_point",
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/stream:stream_handler",
        "//trpc/transport/client:trans_info",
        "//trpc/transport/client/future/common:future_connector_options",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "utils",
    srcs = ["utils.cc"],
    hdrs = ["utils.h"],
    deps = [
        "//trpc/future",
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/runtime/iomodel/reactor/common:socket",
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
        "//trpc/runtime/iomodel/reactor/default:udp_transceiver",
        "//trpc/runtime/threadmodel/common:msg_task",
        "//trpc/transport/client:client_transport_message",
        "//trpc/transport/client:trans_info",
        "//trpc/transport/client/common:client_io_handler_factory",
        "//trpc/transport/client/future/common:future_connection_handler",
        "//trpc/transport/client/future/common:future_connector_options",
        "//trpc/util:ref_ptr",
        "//trpc/util/log:logging",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "future_transport_adapter",
    srcs = ["future_transport_adapter.cc"],
    hdrs = ["future_transport_adapter.h"],
    deps = [
        "//trpc/runtime",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/transport/client/common:client_transport_state",
        "//trpc/transport/client/future:future_connector_group_manager",
        "//trpc/util/thread:latch",
    ],
)
