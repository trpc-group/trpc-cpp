licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "conn_pool",
    hdrs = ["conn_pool.h"],
    deps = [
        "//trpc/transport/client/future/common:future_connector",
        "//trpc/transport/client/future/common:future_connector_options",
        "//trpc/transport/client/future/common:timingwheel_timeout_queue",
        "//trpc/transport/client/future/common:utils",
        "//trpc/util:time",
    ],
)

cc_test(
    name = "conn_pool_test",
    srcs = ["conn_pool_test.cc"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":conn_pool",
        "//trpc/runtime/iomodel/reactor/default:reactor_impl",
        "//trpc/util:net_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "shared_send_queue",
    srcs = ["shared_send_queue.cc"],
    hdrs = ["shared_send_queue.h"],
    deps = [
        "//trpc/transport/client:client_transport_message",
        "//trpc/transport/client/future/common:timingwheel_h",
    ],
)

cc_test(
    name = "shared_send_queue_test",
    srcs = ["shared_send_queue_test.cc"],
    deps = [
        ":shared_send_queue",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "future_conn_pool_message_timeout_handler",
    srcs = ["future_conn_pool_message_timeout_handler.cc"],
    hdrs = ["future_conn_pool_message_timeout_handler.h"],
    deps = [
        ":shared_send_queue",
        "//trpc/transport/client:client_transport_message",
        "//trpc/transport/client:trans_info",
        "//trpc/transport/client/future/common:timingwheel_timeout_queue",
        "//trpc/transport/client/future/common:utils",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "future_tcp_conn_pool_connector_h",
    hdrs = ["future_tcp_conn_pool_connector.h"],
    deps = [
        ":conn_pool",
        ":future_conn_pool_message_timeout_handler",
        "//trpc/transport/client/future/common:future_connector",
        "//trpc/transport/client/future/common:utils",
    ],
)

cc_library(
    name = "future_udp_io_pool_connector_h",
    hdrs = ["future_udp_io_pool_connector.h"],
    deps = [
        ":conn_pool",
        ":future_conn_pool_message_timeout_handler",
        "//trpc/transport/client/future/common:future_connector",
        "//trpc/transport/client/future/common:utils",
    ],
)

cc_library(
    name = "future_conn_pool_connection_handler",
    srcs = ["future_conn_pool_connection_handler.cc"],
    hdrs = ["future_conn_pool_connection_handler.h"],
    deps = [
        ":conn_pool",
        ":future_conn_pool_message_timeout_handler",
        ":future_tcp_conn_pool_connector_h",
        ":future_udp_io_pool_connector_h",
        "//trpc/filter",
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
        "//trpc/transport/client/future/common:future_connection_handler",
        "//trpc/transport/client/future/common:future_connector",
        "//trpc/transport/client/future/common:utils",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "future_conn_pool_connection_handler_factory",
    srcs = ["future_conn_pool_connection_handler_factory.cc"],
    hdrs = ["future_conn_pool_connection_handler_factory.h"],
    deps = [
        ":future_conn_pool_connection_handler",
        "//trpc/util:function",
    ],
)

cc_library(
    name = "future_tcp_conn_pool_connector",
    srcs = ["future_tcp_conn_pool_connector.cc"],
    hdrs = ["future_tcp_conn_pool_connector.h"],
    deps = [
        ":conn_pool",
        ":future_conn_pool_connection_handler_factory",
        ":future_conn_pool_message_timeout_handler",
        "//trpc/filter",
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
        "//trpc/transport/client/future/common:future_connection_handler",
        "//trpc/transport/client/future/common:future_connector",
        "//trpc/transport/client/future/common:utils",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "future_udp_io_pool_connector",
    srcs = ["future_udp_io_pool_connector.cc"],
    hdrs = ["future_udp_io_pool_connector.h"],
    deps = [
        ":conn_pool",
        ":future_conn_pool_connection_handler_factory",
        ":future_conn_pool_message_timeout_handler",
        "//trpc/filter",
        "//trpc/transport/client/future/common:utils",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "future_tcp_conn_pool_connector_group",
    srcs = ["future_tcp_conn_pool_connector_group.cc"],
    hdrs = ["future_tcp_conn_pool_connector_group.h"],
    deps = [
        ":conn_pool",
        ":shared_send_queue",
        ":future_tcp_conn_pool_connector",
        "//trpc/transport/client:fixed_connector_id",
        "//trpc/transport/client/future:future_connector_group",
        "//trpc/transport/client/future:future_connector_group_manager_h",
        "//trpc/util:time",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "future_udp_io_pool_connector_group",
    srcs = ["future_udp_io_pool_connector_group.cc"],
    hdrs = ["future_udp_io_pool_connector_group.h"],
    deps = [
        ":conn_pool",
        ":shared_send_queue",
        ":future_udp_io_pool_connector",
        "//trpc/transport/client:fixed_connector_id",
        "//trpc/transport/client/future:future_connector_group",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)
