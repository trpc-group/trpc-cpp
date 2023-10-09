licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fiber_server_transport",
    srcs = glob(
        ["*.cc"],
        exclude = ["*_test.cc"],
    ),
    hdrs = glob(
        ["*.h"],
        exclude = [
            "fiber_bind_adapter.h",
            "fiber_server_transport_impl.h",
            "fiber_connection_manager.h",
        ],
    ),
    deps = [
        ":fiber_server_connection_handler",
        ":fiber_server_transport_impl_h",
        "//trpc/coroutine:fiber",
        "//trpc/runtime:fiber_runtime",
        "//trpc/runtime/common/stats:frame_stats",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_acceptor",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_reactor",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_tcp_connection",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_udp_transceiver",
        "//trpc/server:server_context",
        "//trpc/transport/server:server_transport",
        "//trpc/transport/server/common:server_connection_handler",
        "//trpc/transport/server/common:server_io_handler_factory",
        "//trpc/util:function",
        "//trpc/util:hash_util",
        "//trpc/util:likely",
        "//trpc/util:random",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    # Breaks dependency cycle：server_stream_connection_handler depends on bind_adapter.
    name = "fiber_bind_adapter_h",
    hdrs = ["fiber_bind_adapter.h"],
    deps = [
        ":fiber_connection_manager_h",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_acceptor",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_tcp_connection",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_udp_transceiver",
        "//trpc/transport/server:server_transport_message",
    ],
)

cc_library(
    # Breaks dependency cycle：server_stream_connection_handler depends on bind_adapter.
    name = "fiber_connection_manager_h",
    hdrs = ["fiber_connection_manager.h"],
)

cc_library(
    name = "fiber_server_connection_handler",
    srcs = ["fiber_server_connection_handler.cc"],
    hdrs = ["fiber_server_connection_handler.h"],
    deps = [
        "//trpc/codec:server_codec",
        "//trpc/stream:stream_handler",
        "//trpc/transport/server/common:server_connection_handler",
        "//trpc/transport/server/fiber:fiber_bind_adapter_h",
        "//trpc/util:function",
    ],
)

cc_library(
    # Breaks dependency cycle：server_stream_connection_handler depends on bind_adapter.
    name = "fiber_server_transport_impl_h",
    hdrs = ["fiber_server_transport_impl.h"],
    deps = [
        ":fiber_bind_adapter_h",
        "//trpc/runtime/iomodel/reactor/common:accept_connection_info",
        "//trpc/transport/server:server_transport",
        "//trpc/util:ref_ptr",
    ],
)

cc_test(
    name = "fiber_server_transport_impl_test",
    srcs = ["fiber_server_transport_impl_test.cc"],
    data = [
        "//trpc/runtime/threadmodel/testing:fiber.yaml",
    ],
    deps = [
        ":fiber_server_transport",
        "//trpc/common/config:trpc_config",
        "//trpc/coroutine:fiber",
        "//trpc/runtime:fiber_runtime",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_reactor",
        "//trpc/util/thread:latch",
        "@com_google_googletest//:gtest_main",
    ],
)
