licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fiber_reactor",
    srcs = ["fiber_reactor.cc"],
    hdrs = ["fiber_reactor.h"],
    deps = [
        "//trpc/coroutine:fiber",
        "//trpc/log:trpc_log",
        "//trpc/runtime:fiber_runtime",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/iomodel/reactor/common:epoll_poller",
        "//trpc/runtime/iomodel/reactor/common:eventfd_notifier",
        "//trpc/util:align",
        "//trpc/util:random",
        "//trpc/util/queue:bounded_mpsc_queue",
    ],
)

cc_library(
    name = "fiber_connection",
    srcs = ["fiber_connection.cc"],
    hdrs = ["fiber_connection.h"],
    deps = [
        "//trpc/coroutine:fiber",
        "//trpc/coroutine:fiber_timer",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/runtime/iomodel/reactor/common:connection",
	"//trpc/runtime/iomodel/reactor/fiber:fiber_reactor",
        "//trpc/util:align",
        "//trpc/util:likely",
        "//trpc/util:ref_ptr",
        "//trpc/util:time",
        "//trpc/util/log:logging",
        "//trpc/util/thread/internal:memory_barrier",
    ],
)

cc_library(
    name = "fiber_acceptor",
    srcs = ["fiber_acceptor.cc"],
    hdrs = ["fiber_acceptor.h"],
    deps = [
        "//trpc/log:trpc_log",
        "//trpc/runtime/iomodel/reactor/common:accept_connection_info",
        "//trpc/runtime/iomodel/reactor/common:network_address",
        "//trpc/runtime/iomodel/reactor/common:unix_address",
        "//trpc/runtime/iomodel/reactor/fiber:fiber_connection",
    ],
)

cc_library(
    name = "writing_buffer_list",
    srcs = ["writing_buffer_list.cc"],
    hdrs = ["writing_buffer_list.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/coroutine:fiber_basic",
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/runtime/iomodel/reactor/common:io_handler",
        "//trpc/runtime/iomodel/reactor/common:io_message",
        "//trpc/util:align",
        "//trpc/util:likely",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/chrono",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "writing_datagram_list",
    srcs = ["writing_datagram_list.cc"],
    hdrs = ["writing_datagram_list.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/runtime/iomodel/reactor/common:io_message",
        "//trpc/runtime/iomodel/reactor/common:network_address",
        "//trpc/runtime/iomodel/reactor/common:socket",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_library(
    name = "fiber_tcp_connection",
    srcs = ["fiber_tcp_connection.cc"],
    hdrs = ["fiber_tcp_connection.h"],
    deps = [
        ":fiber_connection",
        ":writing_buffer_list",
        "//trpc/runtime/iomodel/reactor/common:io_handler",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "fiber_udp_transceiver",
    srcs = ["fiber_udp_transceiver.cc"],
    hdrs = ["fiber_udp_transceiver.h"],
    deps = [
        ":fiber_connection",
        ":writing_datagram_list",
        "//trpc/log:trpc_log",
        "//trpc/runtime/iomodel/reactor/common:network_address",
        "//trpc/util:likely",
    ],
)

cc_test(
    name = "fiber_connection_test",
    srcs = ["fiber_connection_test.cc"],
    deps = [
        ":fiber_connection",
        ":fiber_reactor",
        "//trpc/coroutine:fiber",
        "//trpc/runtime:fiber_runtime",
        "//trpc/util:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_reactor_test",
    srcs = ["fiber_reactor_test.cc"],
    deps = [
        ":fiber_reactor",
        "//trpc/runtime:fiber_runtime",
        "//trpc/util:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_tcp_connection_test",
    srcs = ["fiber_tcp_connection_test.cc"],
    deps = [
        ":fiber_acceptor",
        ":fiber_connection",
        ":fiber_reactor",
        ":fiber_tcp_connection",
        "//trpc/runtime:fiber_runtime",
        "//trpc/runtime/iomodel/reactor/common:default_io_handler",
        "//trpc/util:latch",
	"//trpc/util:time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fiber_udp_transceiver_test",
    srcs = ["fiber_udp_transceiver_test.cc"],
    deps = [
        ":fiber_reactor",
        ":fiber_udp_transceiver",
        "//trpc/runtime:fiber_runtime",
        "//trpc/runtime/iomodel/reactor/common:default_io_handler",
        "//trpc/util:latch",
        "//trpc/util:net_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "writing_buffer_list_test",
    srcs = ["writing_buffer_list_test.cc"],
    deps = [
        ":writing_buffer_list",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "writing_datagram_list_test",
    srcs = ["writing_datagram_list_test.cc"],
    deps = [
        ":writing_datagram_list",
        "//trpc/util:net_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
