licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "future_connector_group",
    hdrs = ["future_connector_group.h"],
    deps = [
        "//trpc/future",
        "//trpc/transport/client/future/common:future_connector",
    ],
)

cc_library(
    name = "future_connector_group_manager_h",
    hdrs = [
        "future_connector_group_manager.h",
    ],
    deps = [
        "//trpc/runtime/iomodel/reactor",
        "//trpc/transport/client/future:future_connector_group",
    ],
)

cc_library(
    name = "future_connector_group_manager",
    srcs = [
        "future_tcp_connector_group_manager.cc",
        "future_udp_connector_group_manager.cc",
    ],
    hdrs = [
        "future_connector_group_manager.h",
        "future_tcp_connector_group_manager.h",
        "future_udp_connector_group_manager.h",
    ],
    deps = [
        ":future_connector_group_manager_h",
        "//trpc/transport/client/future/conn_complex:future_conn_complex_message_timeout_handler",
        "//trpc/transport/client/future/conn_complex:future_tcp_conn_complex_connector_group",
        "//trpc/transport/client/future/conn_complex:future_udp_io_complex_connector_group",
        "//trpc/transport/client/future/conn_pool:future_tcp_conn_pool_connector_group",
        "//trpc/transport/client/future/conn_pool:future_udp_io_pool_connector_group",
        "//trpc/transport/client/future/pipeline:future_tcp_pipeline_connector_group",
    ],
)

cc_library(
    name = "future_transport",
    srcs = ["future_transport.cc"],
    hdrs = ["future_transport.h"],
    deps = [
        "//trpc/future:exception",
        "//trpc/future:future_utility",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime:separate_runtime",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/threadmodel/common:worker_thread",
        "//trpc/stream:stream_handler",
        "//trpc/transport/client:client_transport",
        "//trpc/transport/client:fixed_connector_id",
        "//trpc/transport/client:retry_info_def",
        "//trpc/transport/client/future/common:future_transport_adapter",
        "//trpc/transport/common:connection_handler_manager",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "future_transport_test",
    srcs = ["future_transport_test.cc"],
    data = [
        "//trpc/transport/client/future/testing:merge_separate_threadmodel.yaml",
    ],
    deps = [
        "//trpc/client:client_context",
        "//trpc/common/future:future_utility",
        "//trpc/stream:stream_handler_manager",
        "//trpc/transport/client/future:future_transport",
        "//trpc/transport/client/future/testing:fake_trpc_server",
        "//trpc/transport/common:connection_handler_manager",
        "//trpc/util:unique_id",
        "//trpc/util/thread:latch",
        "@com_google_googletest//:gtest_main",
    ],
)
