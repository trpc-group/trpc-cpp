package(default_visibility = ["//visibility:public"])

cc_library(
    name = "http_async_stream_service",
    srcs = ["http_async_stream_service.cc"],
    hdrs = ["http_async_stream_service.h"],
    deps = [
        "//trpc/codec/http:http_protocol",
        "//trpc/server:rpc_service_impl",
        "//trpc/stream/http/async:stream_function_handler",
        "//trpc/stream/http/async:stream_reader_writer",
        "//trpc/util/http:common",
        "//trpc/util/http:function_handlers",
        "//trpc/util/http:routes",
        "//trpc/util/http:util",
    ],
)

cc_test(
    name = "http_async_stream_service_test",
    srcs = ["http_async_stream_service_test.cc"],
    data = ["//trpc/runtime/threadmodel/testing:merge.yaml"],
    deps = [
        ":http_async_stream_service",
        "//trpc/codec:codec_manager",
        "//trpc/filter/testing:server_filter_testing",
        "//trpc/future:future_utility",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/server:service_adapter",
        "//trpc/server/testing:server_context_testing",
        "//trpc/stream/testing:mock_stream_handler",
        "//trpc/transport/server/testing:server_transport_testing",
        "@com_google_googletest//:gtest_main",
    ],
)
