# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "redis_protocol",
    srcs = ["redis_protocol.cc"],
    hdrs = ["redis_protocol.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/client/redis:reply",
        "//trpc/client/redis:request",
        "//trpc/codec:protocol",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "redis_proto_checker",
    srcs = ["redis_proto_checker.cc"],
    hdrs = ["redis_proto_checker.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":redis_protocol",
        "//trpc/client/redis:reader",      
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "redis_client_codec",
    srcs = ["redis_client_codec.cc"],
    hdrs = ["redis_client_codec.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":redis_proto_checker",
        ":redis_protocol",
        "//trpc/client:client_context",
        "//trpc/client/redis:reply",
        "//trpc/client/redis:request",
        "//trpc/codec:client_codec",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "redis_client_codec_test",
    srcs = ["redis_client_codec_test.cc"],
    deps = [
        ":redis_client_codec",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "redis_proto_checker_test",
    srcs = ["redis_proto_checker_test.cc"],
    deps = [
        ":redis_proto_checker",
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "redis_protocol_test",
    srcs = ["redis_protocol_test.cc"],
    deps = [
        ":redis_protocol",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
