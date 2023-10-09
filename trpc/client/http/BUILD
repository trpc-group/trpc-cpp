licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_test(
    name = "http_fiber_test",
    srcs = ["http_fiber_test.cc"],
    data = [
        "//trpc/client/http/testing:http_fiber.yaml",
        "//trpc/client/http/testing:http_fiber_with_ssl.yaml",
        "//trpc/transport/common/ssl:unit_test_resourses",
    ],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":http_service_proxy",
        "//trpc/client:trpc_client",
        "//trpc/client/http/testing:http_client_call_testing",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/server/testing:fiber_server_testing",
        "//trpc/server/testing:greeter_service_testing",
        "//trpc/server/testing:http_service_testing",
        "//trpc/transport/common/ssl",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_service_proxy",
    srcs = ["http_service_proxy.cc"],
    hdrs = ["http_service_proxy.h"],
    deps = [
        "//trpc/client:rpc_service_proxy",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec/http:http_client_codec",
        "//trpc/common/future",
        "//trpc/stream/http:http_client_stream",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "//trpc/util/log:logging",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "http_service_proxy_test",
    srcs = ["http_service_proxy_test.cc"],
    deps = [
        ":http_service_proxy",
        "//trpc/client:make_client_context",
        "//trpc/client:service_proxy_option_setter",
        "//trpc/codec:codec_manager",
        "//trpc/future:future_utility",
        "//trpc/naming:trpc_naming_registry",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/serialization:trpc_serialization",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "http_stream_proxy",
    srcs = ["http_stream_proxy.cc"],
    hdrs = ["http_stream_proxy.h"],
    deps = [
        "//trpc/client:service_proxy",
        "//trpc/codec/http:http_protocol",
        "//trpc/stream/http/async:stream_reader_writer",
    ],
)

cc_test(
    name = "http_stream_proxy_test",
    srcs = ["http_stream_proxy_test.cc"],
    data = ["//trpc/runtime/threadmodel/testing:merge.yaml"],
    deps = [
        ":http_stream_proxy",
        "//trpc/client:make_client_context",
        "//trpc/client:service_proxy_option_setter",
        "//trpc/codec:codec_manager",
        "//trpc/codec/http:http_client_codec",
        "//trpc/common:trpc_plugin",
        "//trpc/filter/testing:client_filter_testing",
        "//trpc/future:future_utility",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server/testing:service_adapter_testing",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest_main",
    ],
)
