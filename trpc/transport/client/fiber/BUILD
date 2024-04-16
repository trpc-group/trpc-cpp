licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fiber_connector_group_manager",
    srcs = ["fiber_connector_group_manager.cc"],
    hdrs = ["fiber_connector_group_manager.h"],
    deps = [
        ":fiber_connector_group",
        "//trpc/transport/client/common:client_transport_state",
        "//trpc/transport/client/fiber/conn_complex:fiber_conn_complex_impl",
        "//trpc/transport/client/fiber/conn_pool:fiber_conn_pool_impl",
        "//trpc/transport/client/fiber/pipeline:fiber_pipeline_impl",
        "//trpc/util/concurrency:lightly_concurrent_hashmap",
        "//trpc/util/hazptr",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "fiber_connector_group",
    hdrs = ["fiber_connector_group.h"],
    deps = [
        "//trpc/future",
        "//trpc/runtime/iomodel/reactor/common:network_address",
        "//trpc/stream",
        "//trpc/transport/client:client_transport_message",
        "//trpc/transport/client:trans_info",
    ],
)

cc_library(
    name = "fiber_transport",
    srcs = ["fiber_transport.cc"],
    hdrs = ["fiber_transport.h"],
    deps = [
        ":fiber_connector_group",
        ":fiber_connector_group_manager",
        "//trpc/coroutine:fiber",
        "//trpc/runtime:fiber_runtime",
        "//trpc/transport/client:client_transport",
        "//trpc/transport/client/fiber/common:fiber_backup_request_retry",
        "//trpc/util/thread:latch",
    ],
)

cc_test(
    name = "fiber_transport_test",
    srcs = ["fiber_transport_test.cc"],
    deps = [
        ":fiber_transport",
        "//trpc/client/testing:client_context_testing",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec:codec_manager",
        "//trpc/common/future:future_utility",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine:future",
        "//trpc/stream:stream_handler",
        "//trpc/transport/client/fiber/testing:fake_server",
        "//trpc/transport/client/fiber/testing:thread_model_op",
        "@com_google_googletest//:gtest_main",
    ],
)
