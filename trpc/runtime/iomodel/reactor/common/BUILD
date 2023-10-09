licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "epoll",
    srcs = ["epoll.cc"],
    hdrs = ["epoll.h"],
    deps = [
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "epoll_poller",
    srcs = ["epoll_poller.cc"],
    hdrs = ["epoll_poller.h"],
    deps = [
        ":epoll",
        "//trpc/runtime/iomodel/reactor:poller",
    ],
)

cc_library(
    name = "io_message",
    hdrs = ["io_message.h"],
    deps = [
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_library(
    name = "io_handler",
    hdrs = ["io_handler.h"],
    deps = [
    ],
)

cc_library(
    name = "default_io_handler",
    hdrs = ["default_io_handler.h"],
    defines = [] + select({
        "//trpc:trpc_disable_tcp_cork": ["TRPC_DISABLE_TCP_CORK"],
        "//conditions:default": [],
    }),
    deps = [
        ":connection",
        ":io_handler",
    ],
)

cc_library(
    name = "socket",
    srcs = ["socket.cc"],
    hdrs = ["socket.h"],
    defines = [] + select({
        "//trpc:trpc_disable_reuseport": ["TRPC_DISABLE_REUSEPORT"],
        "//trpc:trpc_disable_tcp_cork": ["TRPC_DISABLE_TCP_CORK"],
        "//conditions:default": [],
    }),
    deps = [
        ":network_address",
        ":unix_address",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "network_address",
    srcs = ["network_address.cc"],
    hdrs = ["network_address.h"],
)

cc_library(
    name = "unix_address",
    srcs = ["unix_address.cc"],
    hdrs = ["unix_address.h"],
)

cc_library(
    name = "accept_connection_info",
    hdrs = ["accept_connection_info.h"],
    deps = [
        ":network_address",
        ":socket",
        ":unix_address",
        #"//trpc/util/string:string_util",
    ],
)

cc_library(
    name = "connection_handler",
    hdrs = ["connection_handler.h"],
    deps = [
        ":accept_connection_info",
        ":io_message",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_library(
    name = "notifier",
    hdrs = ["notifier.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor:event_handler",
    ],
)

cc_library(
    name = "eventfd_notifier",
    srcs = ["eventfd_notifier.cc"],
    hdrs = ["eventfd_notifier.h"],
    deps = [
        ":notifier",
        "//trpc/runtime/iomodel/reactor",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "connection",
    hdrs = ["connection.h"],
    deps = [
        ":connection_handler",
        ":io_handler",
        ":io_message",
        ":network_address",
        ":socket",
        "//trpc/runtime/iomodel/reactor:event_handler",
    ],
)

cc_test(
    name = "network_address_test",
    srcs = ["network_address_test.cc"],
    deps = [
        ":network_address",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "unix_address_test",
    srcs = ["unix_address_test.cc"],
    deps = [
        ":unix_address",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "socket_test",
    srcs = ["socket_test.cc"],
    deps = [
        ":socket",
        "//trpc/util:net_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
