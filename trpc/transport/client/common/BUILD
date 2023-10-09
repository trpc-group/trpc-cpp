licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "client_transport_state",
    hdrs = ["client_transport_state.h"],
)

cc_library(
    name = "client_io_handler",
    srcs = ["client_io_handler.cc"],
    hdrs = ["client_io_handler.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/common:io_handler",
        "//trpc/transport/client:trans_info",
    ],
)

cc_library(
    name = "client_io_handler_factory",
    srcs = ["client_io_handler_factory.cc"],
    hdrs = ["client_io_handler_factory.h"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":client_io_handler",
        "//trpc/runtime/iomodel/reactor/common:default_io_handler",
        "//trpc/transport/client:trans_info",
        "//trpc/util/log:logging",
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
    name = "redis_client_io_handler",
    srcs = ["redis_client_io_handler.cc"],
    hdrs = ["redis_client_io_handler.h"],
    deps = [
        "//trpc/client/redis:cmdgen",
        "//trpc/common/config:redis_client_conf",
        "//trpc/runtime/iomodel/reactor/common:io_handler",
        "//trpc/transport/client:trans_info",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
    ],
)