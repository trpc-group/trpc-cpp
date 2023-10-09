licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "future_conn_complex_message_timeout_handler",
    srcs = ["future_conn_complex_message_timeout_handler.cc"],
    hdrs = ["future_conn_complex_message_timeout_handler.h"],
    deps = [
        "//trpc/transport/client:client_transport_message",
        "//trpc/transport/client:trans_info",
        "//trpc/transport/client/future/common:timingwheel_timeout_queue",
        "//trpc/transport/client/future/common:utils",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "future_conn_complex_connection_handler",
    srcs = ["future_conn_complex_connection_handler.cc"],
    hdrs = ["future_conn_complex_connection_handler.h"],
    deps = [
        ":future_conn_complex_message_timeout_handler",
        "//trpc/filter",
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
        "//trpc/transport/client/common:client_transport_state",
        "//trpc/transport/client/future/common:future_connection_handler",
        "//trpc/transport/client/future/common:future_connector",
        "//trpc/transport/client/future/common:utils",
        "//trpc/util:function",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "future_conn_complex_connection_handler_factory",
    srcs = ["future_conn_complex_connection_handler_factory.cc"],
    hdrs = ["future_conn_complex_connection_handler_factory.h"],
    deps = [
        ":future_conn_complex_connection_handler",
        "//trpc/util:function",
    ],
)

cc_library(
    name = "future_tcp_conn_complex_connector",
    srcs = ["future_tcp_conn_complex_connector.cc"],
    hdrs = ["future_tcp_conn_complex_connector.h"],
    deps = [
        ":future_conn_complex_connection_handler_factory",
        "//trpc/filter",
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
        "//trpc/transport/client/future/common:future_connection_handler",
        "//trpc/transport/client/future/common:future_connector",
        "//trpc/transport/client/future/common:utils",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "future_tcp_conn_complex_connector_group",
    srcs = ["future_tcp_conn_complex_connector_group.cc"],
    hdrs = ["future_tcp_conn_complex_connector_group.h"],
    deps = [
        ":future_conn_complex_message_timeout_handler",
        ":future_tcp_conn_complex_connector",
        "//trpc/transport/client:fixed_connector_id",
        "//trpc/transport/client/future:future_connector_group",
        "//trpc/transport/client/future:future_connector_group_manager_h",
        "//trpc/util:time",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "future_udp_io_complex_connector",
    srcs = ["future_udp_io_complex_connector.cc"],
    hdrs = ["future_udp_io_complex_connector.h"],
    deps = [
        ":future_conn_complex_connection_handler_factory",
        "//trpc/transport/client/future/common:future_connection_handler",
        "//trpc/transport/client/future/common:future_connector",
        "//trpc/transport/client/future/common:utils",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "future_udp_io_complex_connector_group",
    srcs = ["future_udp_io_complex_connector_group.cc"],
    hdrs = ["future_udp_io_complex_connector_group.h"],
    deps = [
        ":future_udp_io_complex_connector",
        "//trpc/transport/client/future:future_connector_group",
    ],
)
