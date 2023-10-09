licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rpc_service_method",
    hdrs = ["rpc_service_method.h"],
    deps = [
        "//trpc/server:method",
        "//trpc/server:method_handler",
    ],
)

cc_library(
    name = "unary_rpc_method_handler",
    hdrs = ["unary_rpc_method_handler.h"],
    defines = [] +
              select({
                  "//trpc:trpc_proto_use_arena": ["TRPC_PROTO_USE_ARENA"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/codec:server_codec_factory",
        "//trpc/codec/trpc",
        "//trpc/compressor:trpc_compressor",
        "//trpc/serialization:serialization_factory",
        "//trpc/server:method_handler",
        "//trpc/server:server_context",
        "//trpc/util:pb2json",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/buffer:zero_copy_stream",
        "//trpc/util/log:logging",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "rpc_method_handler",
    hdrs = ["rpc_method_handler.h"],
    deps = [
        ":unary_rpc_method_handler",
    ],
)

cc_library(
    name = "rpc_service_impl",
    srcs = ["rpc_service_impl.cc"],
    hdrs = ["rpc_service_impl.h"],
    deps = [
        "//trpc/codec:codec_helper",
        "//trpc/coroutine:fiber",
        "//trpc/server:service_impl",
        "//trpc/stream:stream_handler",
        "//trpc/util:time",
    ],
)

cc_test(
    name = "rpc_service_impl_test",
    srcs = ["rpc_service_impl_test.cc"],
    deps = [
        ":rpc_method_handler",
        ":rpc_service_impl",
        ":stream_rpc_method_handler",
        "//trpc/codec:codec_manager",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/coroutine/testing:fiber_runtime_testing",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/proto/testing:helloworld_fbs",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server:server_context",
        "//trpc/server/testing:server_context_testing",
        "//trpc/server/testing:service_adapter_testing",
        "//trpc/stream",
        "//trpc/stream/testing:mock_stream_handler",
        "//trpc/stream/testing:mock_stream_provider",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_rpc_method_handler",
    hdrs = ["stream_rpc_method_handler.h"],
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
