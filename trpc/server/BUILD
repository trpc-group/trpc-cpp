licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "http_service",
    srcs = ["http_service.cc"],
    hdrs = ["http_service.h"],
    deps = [
        ":server_context",
        ":service",
        "//trpc/codec/http:http_protocol",
        "//trpc/codec/http:http_server_codec",
        "//trpc/util:deferred",
        "//trpc/util/http:http_handler_groups",
        "//trpc/util/http:routes",
    ],
)

cc_library(
    name = "make_server_context",
    hdrs = ["make_server_context.h"],
    deps = [
        ":server_context",
        "//trpc/codec:server_codec",
        "//trpc/server:service",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "method",
    hdrs = ["method.h"],
)

cc_library(
    name = "method_handler",
    srcs = ["method_handler.cc"],
    hdrs = ["method_handler.h"],
    defines = [] +
              select({
                  "//trpc:trpc_proto_use_arena": ["TRPC_PROTO_USE_ARENA"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/codec:protocol",
        "//trpc/common:status",
        "//trpc/util/buffer:noncontiguous_buffer",
    ] + select({
        "//trpc:trpc_proto_use_arena": [
            "@com_google_protobuf//:protobuf",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "rpc_async_method_handler",
    hdrs = ["rpc_async_method_handler.h"],
    deps = [
        "//trpc/server/async_rpc:async_rpc_method_handler",
    ],
)

cc_library(
    name = "rpc_method_handler",
    hdrs = ["rpc_method_handler.h"],
    deps = [
        "//trpc/server/rpc:rpc_method_handler",
    ],
)

cc_library(
    name = "rpc_service_impl",
    hdrs = ["rpc_service_impl.h"],
    deps = [
        "//trpc/server/async_rpc:async_rpc_service_impl",
        "//trpc/server/rpc:rpc_service_impl",
    ],
)

cc_library(
    name = "rpc_service_method",
    hdrs = ["rpc_service_method.h"],
    deps = [
        "//trpc/server/rpc:rpc_service_method",
    ],
)

cc_library(
    name = "non_rpc_method_handler",
    hdrs = ["non_rpc_method_handler.h"],
    deps = [
        "//trpc/server/non_rpc:non_rpc_method_handler",
    ],
)

cc_library(
    name = "non_rpc_service_impl",
    hdrs = ["non_rpc_service_impl.h"],
    deps = [
        "//trpc/server/non_rpc:non_rpc_service_impl",
    ],
)

cc_library(
    name = "non_rpc_service_method",
    hdrs = ["non_rpc_service_method.h"],
    deps = [
        "//trpc/server/non_rpc:non_rpc_service_method",
    ],
)

cc_library(
    name = "server_context_h",
    hdrs = ["server_context.h"],
    defines = [] +
              select({
                  "//trpc:trpc_proto_use_arena": ["TRPC_PROTO_USE_ARENA"],
                  "//conditions:default": [],
              }),
    deps = [
        ":method_handler",
        "//trpc/codec:protocol",
        "//trpc/codec:server_codec",
        "//trpc/codec/trpc",
        "//trpc/common:status",
        "//trpc/compressor:trpc_compressor",
        "//trpc/coroutine:fiber_local",
        "//trpc/filter:server_filter_controller_h",
        "//trpc/serialization:serialization_type",
        "//trpc/stream:stream_provider",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/flatbuffers:fbs_interface",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_library(
    name = "server_context",
    srcs = ["server_context.cc"],
    deps = [
        ":server_context_h",
        ":service",
        "//trpc/common/config:trpc_config",
        "//trpc/filter:server_filter_controller",
        "//trpc/runtime/common/stats:frame_stats",
        "//trpc/serialization:serialization_factory",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "service_h",
    hdrs = ["service.h"],
    deps = [
        ":service_adapter_option",
        "//trpc/codec:server_codec",
        "//trpc/filter:server_filter_controller_h",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/runtime/iomodel/reactor/default:acceptor",
        "//trpc/server/non_rpc:non_rpc_service_method",
        "//trpc/server/rpc:rpc_service_method",
        "//trpc/transport/server:server_transport",
    ],
)

cc_library(
    name = "service",
    srcs = ["service.cc"],
    deps = [
        ":service_adapter_h",
        ":service_h",
        "//trpc/transport/server:server_transport",
    ],
)

cc_library(
    name = "service_adapter_h",
    hdrs = ["service_adapter.h"],
    deps = [
        ":service_h",
        "//trpc/codec:server_codec_factory",
    ],
)

cc_library(
    name = "service_adapter",
    srcs = ["service_adapter.cc"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":service",
        ":service_adapter_h",
        "//trpc/codec/http:http_protocol",
        "//trpc/filter:server_filter_manager",
        "//trpc/runtime:fiber_runtime",
        "//trpc/runtime:init_runtime",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime:separate_runtime",
        "//trpc/runtime/threadmodel:thread_model_manager",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/runtime/threadmodel/separate:separate_thread_model",
        "//trpc/server:server_context",
        "//trpc/transport/server:server_transport",
        "//trpc/transport/server/default:default_server_transport",
        "//trpc/transport/server/fiber:fiber_server_transport",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ] + select({
        "//trpc:include_ssl": [
            "//trpc/transport/common:ssl_helper",
            "//trpc/transport/common/ssl",
            "//trpc/transport/common/ssl:core",
        ],
        "//trpc:trpc_include_ssl": [
            "//trpc/transport/common:ssl_helper",
            "//trpc/transport/common/ssl",
            "//trpc/transport/common/ssl:core",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "service_adapter_option",
    srcs = ["service_adapter_option.h"],
    deps = [
        "//trpc/common/config:server_conf",
    ],
)

cc_library(
    name = "service_impl",
    srcs = ["service_impl.cc"],
    hdrs = ["service_impl.h"],
    deps = [
        ":server_context",
        ":service",
        "//trpc/codec:codec_helper",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "stream_rpc_async_method_handler",
    hdrs = ["stream_rpc_async_method_handler.h"],
    deps = [
        "//trpc/server/async_rpc:async_stream_rpc_method_handler",
    ],
)

cc_library(
    name = "stream_rpc_method_handler",
    hdrs = ["stream_rpc_method_handler.h"],
    deps = [
        "//trpc/server/rpc:stream_rpc_method_handler",
    ],
)

cc_library(
    name = "trpc_server",
    srcs = ["trpc_server.cc"],
    hdrs = ["trpc_server.h"],
    deps = [
        ":service_adapter",
        "//trpc/admin:admin_service",
        "//trpc/common/config:server_conf",
        "//trpc/coroutine:fiber",
        "//trpc/naming:trpc_naming",
        "//trpc/runtime",
        "//trpc/runtime/common/heartbeat:heartbeat_report",
        "//trpc/runtime/common/stats:frame_stats",
        "//trpc/runtime/threadmodel:thread_model",
        "//trpc/util/chrono:time",
        "//trpc/util/log:logging",
        "//trpc/util/string:string_util",
    ],
)

cc_test(
    name = "trpc_server_test",
    srcs = ["trpc_server_test.cc"],
    data = [
        "//trpc/server/testing:merge_server.yaml",
    ],
    deps = [
        ":trpc_server",
        "//trpc/codec:codec_manager",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/common/config:trpc_config",
        "//trpc/naming/testing:trpc_registry_testing",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime:separate_runtime",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server/testing:greeter_service_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trpc_server_with_stream_rpc_test",
    srcs = ["trpc_server_with_stream_rpc_test.cc"],
    data = [
        "//trpc/server/testing:server_fiber.yaml",
        "//trpc/server/testing:server_fiber_with_ssl.yaml",
        "//trpc/transport/common/ssl:unit_test_resourses",
    ],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        ":trpc_server",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/common/config:trpc_config",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/server:server_context",
        "//trpc/server/testing:fiber_server_testing",
        "//trpc/server/testing:greeter_service_testing",
        "//trpc/server/testing:http_client_testing",
        "//trpc/server/testing:http_service_testing",
        "//trpc/server/testing:trpc_client_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ] + select({
        "//trpc:include_ssl": [
            "//trpc/transport/common:ssl_helper",
            "//trpc/transport/common/ssl",
            "//trpc/transport/common/ssl:core",
        ],
        "//trpc:trpc_include_ssl": [
            "//trpc/transport/common:ssl_helper",
            "//trpc/transport/common/ssl",
            "//trpc/transport/common/ssl:core",
        ],
        "//conditions:default": [],
    }),
)
