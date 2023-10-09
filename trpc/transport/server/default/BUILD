licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "default_server_transport",
    srcs = glob(
        ["*.cc"],
        exclude = ["*_test.cc"],
    ),
    hdrs = glob(
        ["*.h"],
        exclude = [
            "bind_adapter.h",
            "connection_manager.h",
            "server_transport_impl.h",
        ],
    ),
    deps = [
        ":bind_adapter_h",
        ":server_connection_handler",
        "//trpc/common:async_timer",
        "//trpc/runtime/common/stats:frame_stats",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/iomodel/reactor/default:acceptor",
        "//trpc/runtime/iomodel/reactor/default:tcp_acceptor",
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
        "//trpc/runtime/iomodel/reactor/default:udp_transceiver",
        "//trpc/runtime/iomodel/reactor/default:uds_acceptor",
        "//trpc/runtime/threadmodel:thread_model",
        "//trpc/runtime/threadmodel/separate:separate_thread_model",
        "//trpc/server:server_context",
        "//trpc/transport/server:server_transport",
        "//trpc/transport/server/common:server_connection_handler",
        "//trpc/transport/server/common:server_io_handler_factory",
        "//trpc/util:function",
        "//trpc/util:latch",
        "//trpc/util:time",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    # Breaks dependency cycle：server_stream_connection_handler depends on bind_adapter.
    name = "bind_adapter_h",
    hdrs = ["bind_adapter.h"],
    deps = [
        ":connection_manager_h",
        ":server_transport_impl_h",
        "//trpc/common:async_timer",
        "//trpc/runtime",
        "//trpc/runtime/iomodel/reactor/default:udp_transceiver",
    ],
)

cc_library(
    # Breaks dependency cycle：server_stream_connection_handler depends on bind_adapter.
    name = "connection_manager_h",
    hdrs = ["connection_manager.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
    ],
)

cc_test(
    name = "connection_manager_test",
    srcs = ["connection_manager_test.cc"],
    deps = [
        ":default_server_transport",
        "//trpc/runtime/iomodel/reactor/default:reactor_impl",
        "//trpc/runtime/iomodel/reactor/default:tcp_connection",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_connection_handler",
    srcs = ["server_connection_handler.cc"],
    hdrs = ["server_connection_handler.h"],
    deps = [
        "//trpc/codec:server_codec",
        "//trpc/stream:stream_handler",
        "//trpc/transport/server/common:server_connection_handler",
        "//trpc/transport/server/default:bind_adapter_h",
        "//trpc/util:function",
    ],
)

cc_library(
    # Breaks dependency cycle：server_stream_connection_handler depends on bind_adapter.
    name = "server_transport_impl_h",
    hdrs = ["server_transport_impl.h"],
    deps = [
        "//trpc/transport/server:server_transport",
    ],
)

cc_test(
    name = "server_transport_impl_test",
    srcs = ["server_transport_impl_test.cc"],
    data = [
        "//trpc/runtime/threadmodel/testing:separate.yaml",
    ],
    deps = [
        ":default_server_transport",
        "//trpc/runtime:separate_runtime",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/runtime/threadmodel/separate:separate_thread_model",
        "//trpc/util:net_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
