licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "server_io_handler_factory",
    srcs = ["server_io_handler_factory.cc"],
    hdrs = ["server_io_handler_factory.h"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/runtime/iomodel/reactor/common:default_io_handler",
        "//trpc/runtime/iomodel/reactor/common:io_handler",
        "//trpc/transport/server:server_transport_def",
        "//trpc/util:function",
    ] + select({
        "//trpc:include_ssl": [
            "//trpc/transport/common:ssl_helper",
            "//trpc/transport/common:ssl_io_handler",
            "//trpc/transport/common/ssl",
        ],
        "//trpc:trpc_include_ssl": [
            "//trpc/transport/common:ssl_helper",
            "//trpc/transport/common:ssl_io_handler",
            "//trpc/transport/common/ssl",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "server_connection_handler",
    hdrs = ["server_connection_handler.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/transport/server:server_transport_def",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "server_io_handler_factory_test",
    srcs = ["server_io_handler_factory_test.cc"],
    deps = [
        ":server_io_handler_factory",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
