licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "transport_message_common",
    hdrs = ["transport_message_common.h"],
    deps = [
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
    ],
)

cc_library(
    name = "transport_basic_info",
    hdrs = ["transport_basic_info.h"],
    defines = [] +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":transport_message_common",
        "//trpc/util/object_pool:object_pool_ptr",
    ],
)

cc_library(
    name = "transport_message",
    hdrs = ["transport_message.h"],
    deps = [
        "//trpc/transport/server:server_transport_message",
        #"//trpc/transport/client:client_transport_message",
    ],
)

cc_library(
    name = "ssl_helper",
    srcs = ["ssl_helper.cc"],
    hdrs = ["ssl_helper.h"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/common/config:ssl_conf",
        "//trpc/transport/common/ssl",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "ssl_helper_test",
    srcs = ["ssl_helper_test.cc"],
    data = ["//trpc/transport/common/ssl:unit_test_resourses"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ] + select({
        "//trpc:include_ssl": [
            ":ssl_helper",
            "//trpc/common/config:ssl_conf",
            "//trpc/transport/common/ssl",
        ],
        "//trpc:trpc_include_ssl": [
            ":ssl_helper",
            "//trpc/common/config:ssl_conf",
            "//trpc/transport/common/ssl",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "ssl_io_handler",
    srcs = ["ssl_io_handler.cc"],
    hdrs = ["ssl_io_handler.h"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/runtime/iomodel/reactor/common:io_handler",
        "//trpc/util/log:logging",
    ] + select({
        "//trpc:include_ssl": [
            "//trpc/transport/common/ssl",
            "//trpc/transport/common/ssl:core",
        ],
        "//trpc:trpc_include_ssl": [
            "//trpc/transport/common/ssl",
            "//trpc/transport/common/ssl:core",
        ],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "ssl_io_handler_test",
    srcs = ["ssl_io_handler_test.cc"],
    data = ["//trpc/transport/common/ssl:unit_test_resourses"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/server/testing:http_client_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ] + select({
        "//trpc:include_ssl": [
            ":ssl_io_handler",
            "//trpc/transport/common/ssl",
            "//trpc/transport/common/ssl:random",
        ],
        "//trpc:trpc_include_ssl": [
            ":ssl_io_handler",
            "//trpc/transport/common/ssl",
            "//trpc/transport/common/ssl:random",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "connection_handler_manager",
    srcs = ["connection_handler_manager.cc"],
    hdrs = ["connection_handler_manager.h"],
    deps = [
        "//trpc/stream/grpc:grpc_client_stream_connection_handler",
        "//trpc/stream/grpc:grpc_server_stream_connection_handler",
        "//trpc/stream/http:http_client_stream_connection_handler",
        "//trpc/stream/http/async/client:stream_connection_handler",
        "//trpc/stream/http/async/server:stream_connection_handler",
        "//trpc/stream/trpc:trpc_client_stream_connection_handler",
        "//trpc/stream/trpc:trpc_server_stream_connection_handler",
        "//trpc/transport/client/fiber/common:fiber_client_connection_handler_factory",
        "//trpc/transport/client/future/conn_complex:future_conn_complex_connection_handler_factory",
        "//trpc/transport/client/future/conn_pool:future_conn_pool_connection_handler_factory",
        "//trpc/transport/server/default:default_server_transport",
        "//trpc/transport/server/fiber:fiber_server_transport",
    ],
)

cc_test(
    name = "connection_handler_manager_test",
    srcs = ["connection_handler_manager_test.cc"],
    deps = [
        ":connection_handler_manager",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "io_handler_manager",
    srcs = ["io_handler_manager.cc"],
    hdrs = ["io_handler_manager.h"],
    deps = [
        "//trpc/stream/grpc:grpc_io_handler",
        "//trpc/transport/client/common:client_io_handler_factory",
        "//trpc/transport/client/common:redis_client_io_handler",
        "//trpc/transport/server/common:server_io_handler_factory",
    ],
)

cc_test(
    name = "io_handler_manager_test",
    srcs = ["io_handler_manager_test.cc"],
    deps = [
        ":io_handler_manager",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
