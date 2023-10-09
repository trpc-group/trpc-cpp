licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "pipeline_conn_pool",
    hdrs = ["pipeline_conn_pool.h"],
    deps = [
        "//trpc/transport/client/future/common:future_connector",
    ],
)

cc_test(
    name = "pipeline_conn_pool_test",
    srcs = ["pipeline_conn_pool_test.cc"],
    deps = [
        ":pipeline_conn_pool",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "future_pipeline_message_timeout_handler",
    srcs = ["future_pipeline_message_timeout_handler.cc"],
    hdrs = ["future_pipeline_message_timeout_handler.h"],
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
    name = "future_tcp_pipeline_connector",
    srcs = ["future_tcp_pipeline_connector.cc"],
    hdrs = ["future_tcp_pipeline_connector.h"],
    deps = [
        ":future_pipeline_message_timeout_handler",
        ":pipeline_conn_pool",
        "//trpc/filter",
        "//trpc/transport/client/future/common:future_connection_handler",
        "//trpc/transport/client/future/common:future_connector",
        "//trpc/transport/client/future/common:utils",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "future_tcp_pipeline_connector_group",
    srcs = ["future_tcp_pipeline_connector_group.cc"],
    hdrs = ["future_tcp_pipeline_connector_group.h"],
    deps = [
        ":future_tcp_pipeline_connector",
        "//trpc/transport/client/future:future_connector_group",
        "//trpc/transport/client/future:future_connector_group_manager_h",
        "//trpc/util:time",
    ],
)
