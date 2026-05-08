#trpc/server/http_sse/test
cc_test(
    name = "http_sse_stream_parser_test",
    srcs = ["http_sse_stream_parser_test.cc"],
    deps = [
        "//trpc/stream/http:http_sse_stream",      # SseStreamWriter
        "//trpc/util/http/sse:http_sse_parser",         # SseParser target 
        "//trpc/util/http/sse:http_sse",           # sse_event / helpers target 
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/filter:server_filter_controller_h",
        "//trpc/server:server_context_h",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "http_sse_service_test",
    srcs = ["http_sse_service_test.cc"],
    deps = [
        "//trpc/server:http_service",                      # 
        "//trpc/server/http_sse:http_sse_service",         # http_sse service implementation
        "//trpc/server/testing:server_context_testing",    # testing helpers
        "//trpc/transport/server/testing:server_transport_testing",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "//trpc/util/http:http_handler",
        "//trpc/util/http:routes",
        "//trpc/util/http/sse:http_sse",                 # sse types
        "//trpc/stream/http:http_sse_stream",  # SseStreamWriter
        "//trpc/util/http:function_handlers",
        "//trpc/codec:codec_manager",
        "//trpc/serialization:trpc_serialization",
        "@com_google_googletest//:gtest",            
        "@com_google_googletest//:gtest_main",          # gtest
    ],
)
