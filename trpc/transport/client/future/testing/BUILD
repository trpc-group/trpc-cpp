licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files(["merge_separate_threadmodel.yaml"])

cc_library(
    name = "fake_trpc_server",
    hdrs = ["fake_trpc_server.h"],
    deps = [
        "//trpc/codec:codec_manager",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/codec/trpc:trpc_proto_checker",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/common/config:trpc_config",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime:separate_runtime",
        "//trpc/runtime/iomodel/reactor/default:reactor_impl",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/serialization:serialization_factory",
        "//trpc/serialization:serialization_type",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server:make_server_context",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/transport/server/default:default_server_transport",
        "//trpc/util/buffer:zero_copy_stream",
        "@com_google_googletest//:gtest",
    ],
)
