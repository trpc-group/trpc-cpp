load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files(["fiber_client.yaml"])

cc_library(
    name = "client_context_testing",
    hdrs = ["client_context_testing.h"],
    deps = [
        "//trpc/client:client_context",
        "//trpc/codec:client_codec_factory",
    ],
)

cc_library(
    name = "mock_client_transport",
    hdrs = ["mock_client_transport.h"],
    deps = [
        "//trpc/stream:stream_handler",
        "//trpc/transport/client:client_transport",
    ],
)

cc_library(
    name = "service_proxy_testing",
    hdrs = ["service_proxy_testing.h"],
    deps = [
        "//trpc/client:client_context",
        "//trpc/client:service_proxy",
        "//trpc/client/testing:mock_client_transport",
        "//trpc/codec:codec_manager",
        "//trpc/filter:filter_manager",
        "//trpc/naming:selector_factory",
        "//trpc/naming/direct:direct_selector_filter",
        "//trpc/naming/direct:selector_direct",
        "//trpc/runtime",
        "//trpc/serialization:trpc_serialization",
        "@com_google_googletest//:gtest",
    ],
)
