licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "plugin",
    hdrs = ["plugin.h"],
    deps = [
        "//trpc/util:ref_ptr",
    ],
)

cc_test(
    name = "plugin_test",
    srcs = ["plugin_test.cc"],
    deps = [
        ":plugin",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "status",
    srcs = ["status.cc"],
    hdrs = ["status.h"],
)

cc_test(
    name = "status_test",
    srcs = ["status_test.cc"],
    deps = [
        ":status",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "async_timer",
    hdrs = ["async_timer.h"],
    deps = [
        "//trpc/future:async_timer",
    ],
)

cc_library(
    name = "trpc_version",
    srcs = ["trpc_version.cc"],
    hdrs = ["trpc_version.h"],
)

cc_test(
    name = "trpc_version_test",
    srcs = ["trpc_version_test.cc"],
    deps = [
        ":trpc_version",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "runtime_manager",
    srcs = ["runtime_manager.cc"],
    hdrs = ["runtime_manager.h"],
    deps = [
        "//trpc/codec:codec_manager",
        "//trpc/common:trpc_plugin",
        "//trpc/common/config:trpc_config",
        "//trpc/compressor:trpc_compressor",
        "//trpc/config:trpc_conf",
        "//trpc/coroutine:fiber",
        "//trpc/log:trpc_log",
        "//trpc/metrics:trpc_metrics",
        "//trpc/naming:trpc_naming_registry",
        "//trpc/runtime",
        "//trpc/runtime:fiber_runtime",
        "//trpc/runtime:runtime_state",
        "//trpc/runtime:separate_runtime",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/serialization:trpc_serialization",
        "//trpc/telemetry:trpc_telemetry",
        "//trpc/tracing:trpc_tracing",
        "//trpc/transport/common:connection_handler_manager",
        "//trpc/transport/common:io_handler_manager",
        "//trpc/transport/common:ssl_helper",
        "//trpc/util:net_util",
        "//trpc/util/internal:time_keeper",
        "//trpc/util/thread:latch",
    ],
)

cc_library(
    name = "trpc_plugin",
    srcs = ["trpc_plugin.cc"],
    hdrs = ["trpc_plugin.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/auth",
        "//trpc/auth:auth_factory",
        "//trpc/auth:auth_center_follower_factory",
        #"//trpc/auth:auth_manager",
        "//trpc/client:trpc_client",
        "//trpc/codec:codec_manager",
        "//trpc/codec:server_codec_factory",
        "//trpc/compressor",
        "//trpc/compressor:compressor_factory",
        "//trpc/compressor:trpc_compressor",
        "//trpc/config:trpc_conf",
        "//trpc/filter:filter_manager",
        "//trpc/filter:trpc_filter",
        "//trpc/log:trpc_log",
        "//trpc/metrics:trpc_metrics",
        "//trpc/naming:trpc_naming_registry",
        "//trpc/runtime",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime:separate_runtime",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/runtime/common/runtime_info_report:runtime_info_reporter",
        "//trpc/runtime/common/stats:frame_stats",
        "//trpc/serialization",
        "//trpc/serialization:serialization_factory",
        "//trpc/serialization:trpc_serialization",
        "//trpc/stream:stream_handler_manager",
        #"//trpc/stream/trpc:trpc_stream_handler",
        "//trpc/telemetry:telemetry_factory",
        "//trpc/telemetry:trpc_telemetry",
        "//trpc/tracing:tracing_factory",
        "//trpc/tracing:trpc_tracing",
        "//trpc/transport/common:connection_handler_manager",
        "//trpc/transport/common:io_handler_manager",
        "//trpc/transport/common:ssl_helper",
        "//trpc/util/log/default:default_log",
        "//trpc/util:net_util",
        "//trpc/overload_control:trpc_overload_control",
        "//trpc/naming/common/util/loadbalance:trpc_load_balance"
    ] + select({
        "//trpc:trpc_include_rpcz": [
            "//trpc/rpcz:collector",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "trpc_app",
    srcs = ["trpc_app.cc"],
    hdrs = ["trpc_app.h"],
    deps = [
        ":trpc_plugin",
        ":trpc_version",
        "//trpc/client:make_client_context",
        "//trpc/client:trpc_client",
        "//trpc/common/config:trpc_config",
        "//trpc/common/logging:trpc_logging",
        "//trpc/config:trpc_conf",
        "//trpc/filter:server_filter_manager",
        #"//trpc/overload_control/flow_control:flow_controller_factory",
        "//trpc/runtime",
        "//trpc/server:trpc_server",
        "//trpc/tvar/common:write_mostly",
        "//trpc/util/chrono",
        "//trpc/util/log:logging",
        "//trpc/util:latch",
        "//trpc/util:pb2json",
        "//trpc/util:time",
        "@com_github_gflags_gflags//:gflags",
    ],
)

cc_test(
    name = "runtime_manager_fiber_test",
    srcs = ["runtime_manager_fiber_test.cc"],
    data = [
        "//trpc/common/testing:fiber_testing.yaml",
    ],
    deps = [
        ":runtime_manager",
        "//trpc/common/config:trpc_config",
        "//trpc/util:latch",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "runtime_manager_thread_test",
    srcs = ["runtime_manager_thread_test.cc"],
    data = [
        "//trpc/common/testing:merge_testing.yaml",
    ],
    deps = [
        ":runtime_manager",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
