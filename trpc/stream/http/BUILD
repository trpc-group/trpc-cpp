licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common",
    srcs = ["common.cc"],
    hdrs = ["common.h"],
    deps = [
        "//trpc/common:status",
        "//trpc/stream:stream_provider",
    ],
)

cc_test(
    name = "common_test",
    srcs = ["common_test.cc"],
    deps = [
        ":common",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_client_stream",
    srcs = ["http_client_stream.cc"],
    hdrs = ["http_client_stream.h"],
    deps = [
        ":http_stream",
        ":http_stream_provider",
        "//trpc/client:client_context",
        "//trpc/codec/http:http_protocol",
        "//trpc/filter:client_filter_controller",
        "//trpc/stream:stream_handler",
        "//trpc/util/http:common",
        "//trpc/util/http:reply",
        "//trpc/util/http:util",
        "//trpc/util/http/sse:http_sse",
        "//trpc/codec/http_sse:http_sse_protocol",
    ],
)

cc_test(
    name = "http_client_stream_test",
    srcs = ["http_client_stream_test.cc"],
    deps = [
        ":http_client_stream",
        ":http_client_stream_handler",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/util/http/sse:http_sse",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_client_stream_connection_handler",
    srcs = ["http_client_stream_connection_handler.cc"],
    hdrs = ["http_client_stream_connection_handler.h"],
    deps = [
        ":http_client_stream",
        ":http_client_stream_handler",
        "//trpc/coroutine:fiber",
        "//trpc/stream:client_stream_handler_factory",
        "//trpc/stream:stream_handler",
        "//trpc/transport/client/fiber/common:fiber_client_connection_handler",
        "//trpc/util/http/stream:http_client_stream_response",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "http_client_stream_connection_handler_test",
    srcs = ["http_client_stream_connection_handler_test.cc"],
    deps = [
        ":http_client_stream_connection_handler",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec:codec_manager",
        "//trpc/codec/testing:client_codec_testing",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/http:http_client_stream_handler",
        "//trpc/stream/testing:mock_connection",
        "//trpc/stream/trpc/testing:trpc_stream_testing",
        "//trpc/util/http/stream:http_client_stream_response",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_client_stream_handler",
    srcs = ["http_client_stream_handler.cc"],
    hdrs = ["http_client_stream_handler.h"],
    deps = [
        ":http_client_stream",
        ":http_stream_provider",
        "//trpc/coroutine:fiber",
        "//trpc/filter:client_filter_controller",
        "//trpc/stream:stream_handler",
        "//trpc/stream:stream_message",
    ],
)

cc_test(
    name = "http_client_stream_handler_test",
    srcs = ["http_client_stream_handler_test.cc"],
    deps = [
        ":http_client_stream_handler",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_stream",
    deps = [
        ":http_stream_impl",
        "//trpc/util/http:response",
    ],
)

cc_library(
    name = "http_stream_hdrs",
    hdrs = ["http_stream.h"],
    deps = [
        ":common",
        "//trpc/common:status",
        "//trpc/coroutine:fiber",
        "//trpc/server:server_context",
        "//trpc/stream:stream_handler",
        "//trpc/util/http:util",
        "//trpc/util/http/sse:http_sse",
    ],
)

cc_library(
    name = "http_stream_impl",
    srcs = ["http_stream.cc"],
    deps = [
        ":http_stream_hdrs",
        "//trpc/util/http:request",
        "//trpc/util/http:response_hdrs",
    ],
)

cc_test(
    name = "http_stream_test",
    srcs = ["http_stream_test.cc"],
    deps = [
        ":http_stream",
        "//trpc/codec:codec_manager",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server:http_service",
        "//trpc/server/testing:server_context_testing",
        "//trpc/transport/server/testing:server_transport_testing",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
cc_library(
    name = "http_sse_stream",
    srcs = ["http_sse_stream.cc"],
    hdrs = ["http_sse_stream.h"],
    deps = [
        "//trpc/common:status",
        "//trpc/server:server_context",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/http/sse:http_sse",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "//trpc/util/http:util",
        "//trpc/codec/http_sse:http_sse_protocol",   # for HttpSseResponseProtocol
        "//trpc/codec/http_sse:http_sse_server_codec",  # 如果WriteHeader用到
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "http_sse_stream_test",
    srcs = ["http_sse_stream_test.cc"],
    deps = [
        ":http_sse_stream",
        "//trpc/codec:codec_manager",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server:http_service",
        "//trpc/util/http/sse:http_sse",
        "//trpc/server/testing:server_context_testing",
        "//trpc/transport/server/testing:server_transport_testing",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
cc_library(
    name = "http_stream_provider",
    hdrs = ["http_stream_provider.h"],
    deps = [
        "//trpc/coroutine:fiber",
        "//trpc/stream:stream_provider",
        "//trpc/stream/http:common",
    ],
)
