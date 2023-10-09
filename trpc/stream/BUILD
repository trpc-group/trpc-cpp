licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "client_stream_handler_factory",
    hdrs = ["client_stream_handler_factory.h"],
    deps = [
        ":stream_handler",
    ],
)

cc_library(
    name = "common_stream",
    srcs = ["common_stream.cc"],
    hdrs = ["common_stream.h"],
    deps = [
        ":fiber_stream_job_scheduler",
        ":stream_var",
        "//trpc/codec:protocol",
        "//trpc/common:async_timer",
        "//trpc/common/future:future_utility",
        "//trpc/util:function",
    ],
)

cc_test(
    name = "common_stream_test",
    srcs = ["common_stream_test.cc"],
    deps = [
        ":common_stream",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "fiber_stream_job_scheduler",
    srcs = ["fiber_stream_job_scheduler.cc"],
    hdrs = ["fiber_stream_job_scheduler.h"],
    deps = [
        ":stream_handler",
        ":stream_message",
        ":stream_var",
        "//trpc/codec:protocol",
        "//trpc/coroutine:fiber",
    ],
)

cc_test(
    name = "fiber_stream_job_scheduler_test",
    srcs = ["fiber_stream_job_scheduler_test.cc"],
    deps = [
        ":fiber_stream_job_scheduler",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_stream_handler_factory",
    hdrs = ["server_stream_handler_factory.h"],
    deps = [
        ":stream_handler",
    ],
)

cc_library(
    name = "stream",
    srcs = [],
    hdrs = [
        "stream.h",
        "stream_async.h",
    ],
    deps = [
        ":stream_provider",
        "//trpc/codec/trpc",
        "//trpc/common:status",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/flatbuffers:fbs_interface",
        "//trpc/util/log:logging",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "stream_test",
    srcs = ["stream_test.cc"],
    deps = [
        ":stream",
        "//trpc/serialization",
        "//trpc/serialization:serialization_factory",
        "//trpc/serialization:serialization_type",
        "//trpc/serialization:trpc_serialization",
        "//trpc/stream/testing:mock_stream_provider",
        "//trpc/stream/testing:stream_proto",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "stream_async_test",
    srcs = ["stream_async_test.cc"],
    deps = [
        ":stream",
        "//trpc/serialization",
        "//trpc/serialization:serialization_factory",
        "//trpc/serialization:serialization_type",
        "//trpc/serialization:trpc_serialization",
        "//trpc/stream/testing:mock_stream_provider",
        "//trpc/stream/testing:stream_proto",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_handler",
    srcs = [],
    hdrs = ["stream_handler.h"],
    deps = [
        "//trpc/common:status",
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/runtime/iomodel/reactor/common:io_message",
        "//trpc/stream:stream_message",
        "//trpc/stream:stream_provider",
        "//trpc/transport/server:server_transport_def",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_library(
    name = "stream_handler_manager",
    srcs = ["stream_handler_manager.cc"],
    hdrs = ["stream_handler_manager.h"],
    deps = [
        ":client_stream_handler_factory",
        ":server_stream_handler_factory",
        "//trpc/stream/grpc:grpc_client_stream_handler",
        "//trpc/stream/grpc:grpc_server_stream_handler",
        "//trpc/stream/http:http_client_stream_handler",
        "//trpc/stream/http/async/client:stream_handler",
        "//trpc/stream/http/async/server:stream_handler",
        "//trpc/stream/trpc:trpc_client_stream_handler",
        "//trpc/stream/trpc:trpc_server_stream_handler",
    ],
)

cc_test(
    name = "stream_handler_manager_test",
    srcs = ["stream_handler_manager_test.cc"],
    deps = [
        ":stream_handler_manager",
        "//trpc/codec:codec_manager",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_message",
    hdrs = ["stream_message.h"],
)

cc_library(
    name = "stream_provider",
    hdrs = ["stream_provider.h"],
    deps = [
        "//trpc/common:status",
        "//trpc/common/future",
        "//trpc/serialization",
        "//trpc/util:ref_ptr",
        "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_test(
    name = "stream_provider_test",
    srcs = ["stream_provider_test.cc"],
    deps = [
        ":stream_provider",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "stream_var",
    srcs = ["stream_var.cc"],
    hdrs = ["stream_var.h"],
    deps = [
        "//trpc/coroutine:fiber",
        "//trpc/metrics:metrics_factory",
        "//trpc/tvar/basic_ops:reducer",
        "//trpc/tvar/compound_ops:latency_recorder",
    ],
)

cc_test(
    name = "stream_var_test",
    srcs = ["stream_var_test.cc"],
    deps = [
        ":stream_var",
        "//trpc/coroutine/testing:fiber_runtime_test",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
    deps = [
        "//trpc/codec:server_codec",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/server:server_context",
        "//trpc/util:time",
    ],
)
