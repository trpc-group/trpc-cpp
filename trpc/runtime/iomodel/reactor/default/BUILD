licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "acceptor",
    hdrs = ["acceptor.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor:event_handler",
        "//trpc/runtime/iomodel/reactor/common:accept_connection_info",
    ],
)

cc_library(
    name = "tcp_acceptor",
    srcs = ["tcp_acceptor.cc"],
    hdrs = ["tcp_acceptor.h"],
    deps = [
        ":acceptor",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/iomodel/reactor/common:accept_connection_info",
        "//trpc/runtime/iomodel/reactor/common:network_address",
        "//trpc/runtime/iomodel/reactor/common:socket",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "uds_acceptor",
    srcs = ["uds_acceptor.cc"],
    hdrs = ["uds_acceptor.h"],
    deps = [
        ":acceptor",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/iomodel/reactor/common:accept_connection_info",
        "//trpc/runtime/iomodel/reactor/common:socket",
        "//trpc/runtime/iomodel/reactor/common:unix_address",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "tcp_connection",
    srcs = ["tcp_connection.cc"],
    hdrs = ["tcp_connection.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/runtime/iomodel/reactor/common:io_handler",
        "//trpc/runtime/iomodel/reactor/common:io_message",
        "//trpc/runtime/iomodel/reactor/common:socket",
        "//trpc/util:align",
        "//trpc/util:time",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "udp_transceiver",
    srcs = ["udp_transceiver.cc"],
    hdrs = ["udp_transceiver.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/runtime/iomodel/reactor/common:io_message",
        "//trpc/runtime/iomodel/reactor/common:socket",
        "//trpc/util:align",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "timer_queue",
    srcs = ["timer_queue.cc"],
    hdrs = ["timer_queue.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/threadmodel/common:timer_task",
        "//trpc/util:check",
        "//trpc/util:time",
        "//trpc/util/log:logging",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "reactor_impl",
    srcs = ["reactor_impl.cc"],
    hdrs = ["reactor_impl.h"],
    defines = select({
        "//trpc:trpc_include_async_io": ["TRPC_BUILD_INCLUDE_ASYNC_IO"],
        "//conditions:default": [],
    }),
    deps = [
        ":timer_queue",
        "//trpc/runtime/common/heartbeat:heartbeat_info",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/iomodel/reactor:event_handler",
        "//trpc/runtime/iomodel/reactor/common:epoll_poller",
        "//trpc/runtime/iomodel/reactor/common:eventfd_notifier",
        "//trpc/util:align",
        "//trpc/util:time",
        "//trpc/util/log:logging",
        "//trpc/util/queue:bounded_mpsc_queue",
    ] + select({
        "//trpc:trpc_include_async_io": [
            "//trpc/util/async_io",
        ],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "reactor_impl_test",
    srcs = ["reactor_impl_test.cc"],
    deps = [
        ":reactor_impl",
        "//trpc/util/thread:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "tcp_acceptor_test",
    srcs = ["tcp_acceptor_test.cc"],
    deps = [
        ":tcp_acceptor",
        "//trpc/runtime/iomodel/reactor/default:reactor_impl",
        "//trpc/util:net_util",
        "//trpc/util/thread:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "tcp_connection_test",
    srcs = ["tcp_connection_test.cc"],
    deps = [
        ":reactor_impl",
        ":tcp_acceptor",
        ":tcp_connection",
        "//trpc/runtime/iomodel/reactor/common:default_io_handler",
        "//trpc/util:net_util",
        "//trpc/util:time",
        "//trpc/util/thread:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "timer_queue_test",
    srcs = ["timer_queue_test.cc"],
    deps = [
        ":reactor_impl",
        ":timer_queue",
        "//trpc/util/thread:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "udp_transceiver_test",
    srcs = ["udp_transceiver_test.cc"],
    deps = [
        ":reactor_impl",
        ":udp_transceiver",
        "//trpc/runtime/iomodel/reactor/common:default_io_handler",
        "//trpc/util:net_util",
        "//trpc/util/thread:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "uds_acceptor_test",
    srcs = ["uds_acceptor_test.cc"],
    deps = [
        ":reactor_impl",
        ":uds_acceptor",
        "//trpc/util:net_util",
        "//trpc/util/thread:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
