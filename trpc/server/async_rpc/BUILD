licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "async_rpc_method_handler",
    hdrs = ["async_rpc_method_handler.h"],
    deps = [
        "//trpc/future",
        "//trpc/server/rpc:unary_rpc_method_handler",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "async_rpc_service_impl",
    srcs = ["async_rpc_service_impl.cc"],
    hdrs = ["async_rpc_service_impl.h"],
    deps = [
        "//trpc/server:service_impl",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "async_rpc_service_impl_test",
    srcs = ["async_rpc_service_impl_test.cc"],
    deps = [
        ":async_rpc_method_handler",
        ":async_rpc_service_impl",
        ":async_stream_rpc_method_handler",
        "//trpc/codec:codec_manager",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server:server_context",
        "//trpc/server/testing:server_context_testing",
        "//trpc/stream/testing:mock_stream_provider",
        "//trpc/transport/server/testing:server_transport_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "async_stream_rpc_method_handler",
    hdrs = ["async_stream_rpc_method_handler.h"],
    deps = [
        "//trpc/codec:server_codec_factory",
        "//trpc/codec/trpc",
        "//trpc/serialization:serialization_factory",
        "//trpc/server:method",
        "//trpc/server:method_handler",
        "//trpc/server:server_context",
        "//trpc/stream",
        "//trpc/util/log:logging",
    ],
)
