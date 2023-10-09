licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "forward_rpc_method_handler",
    hdrs = ["forward_rpc_method_handler.h"],
    deps = [
        "//trpc/server:method_handler",
        "//trpc/server:server_context",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "forward_rpc_service_impl",
    srcs = ["forward_rpc_service_impl.cc"],
    hdrs = ["forward_rpc_service_impl.h"],
    deps = [
        "//trpc/server:service_impl",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "forward_rpc_service_impl_test",
    srcs = ["forward_rpc_service_impl_test.cc"],
    deps = [
        ":forward_rpc_method_handler",
        ":forward_rpc_service_impl",
        "//trpc/codec:codec_manager",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server:server_context",
        "//trpc/server/testing:server_context_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
