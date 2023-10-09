licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mock_server_transport",
    hdrs = ["mock_server_transport.h"],
    srcs = [],
    deps = [
        "//trpc/transport/server:server_transport",
        "@com_google_googletest//:gtest",
    ],
)

exports_files(["merge_server.yaml"])

cc_library(
    name = "fiber_server_testing",
    srcs = ["fiber_server_testing.h"],
    deps = [
        "//trpc/codec:codec_manager",
        "//trpc/common/config:trpc_config",
        "//trpc/coroutine:fiber",
        "//trpc/naming:trpc_naming_registry",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server:trpc_server",
        "//trpc/stream:stream_handler_manager",
        "//trpc/transport/common:connection_handler_manager",
        "//trpc/transport/common:io_handler_manager",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "server_context_testing",
    hdrs = ["server_context_testing.h"],
    deps = [
        "//trpc/codec:server_codec_factory",
        "//trpc/server:make_server_context",
        "//trpc/server:server_context_h",
        "//trpc/server:service_h",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "greeter_service_testing",
    srcs = ["greeter_service_testing.cc"],
    hdrs = ["greeter_service_testing.h"],
    deps = [
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/server:rpc_async_method_handler",
        "//trpc/server:rpc_method_handler",
        "//trpc/server/rpc:rpc_service_impl",
        "//trpc/server/rpc:stream_rpc_method_handler",
    ],
)

cc_library(
    name = "http_client_testing",
    srcs = ["http_client_testing.cc"],
    hdrs = ["http_client_testing.h"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":stream_transport_testing",
        "//trpc/runtime/iomodel/reactor/common:socket",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/http:http_parser",
    ] + select({
           "//trpc:include_ssl": [
               "//trpc/transport/common:ssl_helper",
               "//trpc/transport/common/ssl",
               "//trpc/transport/common/ssl:core",
           ],
           "//trpc:trpc_include_ssl": [
               "//trpc/transport/common:ssl_helper",
               "//trpc/transport/common/ssl",
               "//trpc/transport/common/ssl:core",
           ],
           "//conditions:default": [],
       }),
)

cc_library(
    name = "http_service_testing",
    srcs = ["http_service_testing.cc"],
    hdrs = ["http_service_testing.h"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":http_client_testing",
        "//trpc/util/http:function_handlers",
        "//trpc/util/http:http_handler",
        "//trpc/util/http:http_handler_groups",
        "//trpc/util/http:routes",
    ],
)

exports_files(["server_fiber.yaml", "server_fiber_with_ssl.yaml"])

cc_library(
    name = "stream_transport_testing",
    srcs = ["stream_transport_testing.cc"],
    hdrs = ["stream_transport_testing.h"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/runtime/iomodel/reactor/common:socket",
        "//trpc/util/buffer:noncontiguous_buffer",
    ] + select({
            "//trpc:include_ssl": [
                "//trpc/transport/common:ssl_helper",
                "//trpc/transport/common/ssl",
                "//trpc/transport/common/ssl:core",
            ],
            "//trpc:trpc_include_ssl": [
                "//trpc/transport/common:ssl_helper",
                "//trpc/transport/common/ssl",
                "//trpc/transport/common/ssl:core",
            ],
            "//conditions:default": [],
        }),
)

cc_library(
    name = "trpc_client_testing",
    srcs = ["trpc_client_testing.cc"],
    hdrs = ["trpc_client_testing.h"],
    deps = [
        ":stream_transport_testing",
        "//trpc/codec/trpc",
        "//trpc/codec/trpc:trpc_proto_checker",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/runtime/iomodel/reactor/common:socket",
        "//trpc/runtime/iomodel/reactor/testing:mock_connection_testing",
        "//trpc/stream:stream_message",
        "//trpc/stream:stream_provider",
        "//trpc/stream/trpc/testing:trpc_stream_testing",
        "//trpc/util:unique_id",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_library(
    name = "service_adapter_testing",
    hdrs = ["service_adapter_testing.h"],
    deps = [
        "//trpc/server:server_context",
        "//trpc/server:service",
        "//trpc/server:service_adapter",
        "//trpc/transport/common:transport_message",
        "//trpc/util:time",
    ],
)
