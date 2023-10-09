licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "non_rpc_service_method",
    hdrs = ["non_rpc_service_method.h"],
    deps = [
        "//trpc/server:method",
        "//trpc/server:method_handler",
    ],
)

cc_library(
    name = "non_rpc_method_handler",
    hdrs = ["non_rpc_method_handler.h"],
    deps = [
        "//trpc/server:method_handler",
        "//trpc/server:server_context",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "non_rpc_service_impl",
    srcs = ["non_rpc_service_impl.cc"],
    hdrs = ["non_rpc_service_impl.h"],
    deps = [
        "//trpc/codec:codec_helper",
        "//trpc/server:service_impl",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "simple_non_rpc_service_impl",
    hdrs = ["simple_non_rpc_service_impl.h"],
    deps = [
        ":non_rpc_method_handler",
        ":non_rpc_service_impl",
        "//trpc/codec:server_codec",
    ],
)

cc_test(
    name = "non_rpc_service_impl_test",
    srcs = ["non_rpc_service_impl_test.cc"],
    deps = [
        ":non_rpc_method_handler",
        ":non_rpc_service_impl",
        "//trpc/codec:codec_manager",
        "//trpc/codec/testing:protocol_testing",
        "//trpc/codec/testing:server_codec_testing",
        "//trpc/server:server_context",
        "//trpc/server/testing:server_context_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
