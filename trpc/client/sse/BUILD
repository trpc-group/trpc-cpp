cc_library(
    name = "http_sse_proxy",
    srcs = ["http_sse_proxy.cc"],
    hdrs = ["http_sse_proxy.h"],
    deps = [
        "//trpc/client:client_context",
        ":sse_stream_reader",
        "//trpc/client:service_proxy",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec/http:http_client_codec",
        "//trpc/codec/http:http_protocol",
        "//trpc/common:status",
        "//trpc/common/config:trpc_config",
        "//trpc/stream:stream_provider",
        "//trpc/stream/http:http_stream_provider",
        "//trpc/stream/http:http_client_stream",
        "//trpc/util/http:util",
        "//trpc/util/http:url",
        "//trpc/util/log:logging",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "sse_stream_reader",
    srcs = ["http_sse_stream_reader.cc"],
    hdrs = ["http_sse_stream_reader.h"],
    deps = [
        "//trpc/client:client_context",
        "//trpc/stream:stream_provider",
        "//trpc/stream/http:http_stream_provider",
        "//trpc/stream/http:http_client_stream",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/http/sse:http_sse_parser",
        "//trpc/util/http:http_header",
        "//trpc/util:string_helper",
        "//trpc/util/log:logging",
        "//trpc/common:status",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "http_sse_proxy_test",
    srcs = ["http_sse_proxy_test.cc"],
    deps = [
        ":http_sse_proxy",
        "//trpc/client:make_client_context",
        "//trpc/client:service_proxy_option_setter",
        "//trpc/codec:codec_manager",
        "//trpc/codec/http:http_client_codec",
        "//trpc/common/config:trpc_config",
        "//trpc/filter/testing:client_filter_testing",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/serialization:trpc_serialization",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "http_sse_stream_reader_test",
    srcs = ["http_sse_stream_reader_test.cc"],
    deps = [
        ":sse_stream_reader",
        "//trpc/stream/testing:mock_stream_provider",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/http/sse:http_sse",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
