licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "client_context",
    srcs = ["client_context.cc"],
    hdrs = ["client_context.h"],
    deps = [
        "//trpc/client:service_proxy_option",
        "//trpc/codec:client_codec",
        "//trpc/codec:protocol",
        "//trpc/common:status",
        "//trpc/compressor:compressor_type",
        "//trpc/filter:filter_point",
        "//trpc/naming/common:common_inc_deprecated",
        "//trpc/naming/common:constants",
        "//trpc/transport/client:retry_info_def",
        "//trpc/transport/common:transport_message_common",
        "//trpc/util:ref_ptr",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "service_proxy_option",
    hdrs = ["service_proxy_option.h"],
    deps = [
        "//trpc/common/config:default_value",
        "//trpc/common/config:redis_client_conf",
        "//trpc/common/config:mysql_client_conf",
        "//trpc/common/config:ssl_conf",
        "//trpc/naming/common:common_inc_deprecated",
        "//trpc/runtime/iomodel/reactor/common:connection_handler",
        "//trpc/runtime/iomodel/reactor/common:socket",
        "//trpc/transport/client:trans_info",
    ],
)

cc_library(
    name = "service_proxy_option_setter",
    srcs = ["service_proxy_option_setter.cc"],
    hdrs = ["service_proxy_option_setter.h"],
    deps = [
        "//trpc/client:service_proxy_option",
        "//trpc/common/config:redis_client_conf",
        "//trpc/common/config:mysql_client_conf",
        "//trpc/common/config:ssl_conf",
    ],
)

cc_test(
    name = "service_proxy_option_setter_test",
    srcs = ["service_proxy_option_setter_test.cc"],
    linkopts = ["-ldl"],
    deps = [
        ":service_proxy_option_setter",
        "//trpc/runtime/iomodel/reactor/common:connection",
        "//trpc/runtime/iomodel/reactor/testing:mock_connection_testing",
        "//trpc/util/buffer:noncontiguous_buffer",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "service_proxy",
    srcs = ["service_proxy.cc"],
    hdrs = ["service_proxy.h"],
    defines = [] + select({
        "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
        "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
        "//conditions:default": [],
    }),
    deps = [
        ":service_proxy_option",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/common/config:client_conf",
        "//trpc/common/config:default_value",
        "//trpc/common/future",
        "//trpc/common:status",
        "//trpc/filter",
        "//trpc/filter:client_filter_controller",
        "//trpc/filter:filter_manager",
        "//trpc/naming:trpc_naming",
        "//trpc/runtime/common/stats:frame_stats",
        "//trpc/runtime:fiber_runtime",
        "//trpc/runtime:init_runtime",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime:separate_runtime",
        "//trpc/transport/client:client_transport",
        "//trpc/transport/client:preallocation_option",
        "//trpc/transport/client/future:future_transport",
        "//trpc/transport/client/fiber:fiber_transport",
        "//trpc/tvar/basic_ops:reducer",
        "//trpc/stream:stream_handler",
        "//trpc/serialization:serialization_factory",
        "//trpc/util:unique_id",
        "//trpc/util:net_util",
        "//trpc/util:time",
        "//trpc/util/string:string_util",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util:domain_util",
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

cc_test(
    name = "service_proxy_test",
    srcs = ["service_proxy_test.cc"],
    deps = [
        ":service_proxy",
        ":service_proxy_option_setter",
        "//trpc/client:make_client_context",
        "//trpc/client/testing:mock_client_transport",
        "//trpc/client/testing:service_proxy_testing",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/filter:client_filter_manager",
        "//trpc/future:future_utility",
        "//trpc/naming:trpc_naming",
        "//trpc/stream:stream_provider",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "rpc_service_proxy",
    srcs = ["rpc_service_proxy.cc"],
    hdrs = ["rpc_service_proxy.h"],
    deps = [
        ":service_proxy",
        "//trpc/codec:client_codec",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec:codec_helper",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/serialization:serialization_factory",
        "//trpc/serialization:serialization_type",
        "//trpc/util/flatbuffers:fbs_interface",
        "//trpc/util/log:logging",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_test(
    name = "rpc_service_proxy_test",
    srcs = ["rpc_service_proxy_test.cc"],
    deps = [
        ":rpc_service_proxy",
        ":service_proxy_option_setter",
        "//trpc/client:make_client_context",
        "//trpc/client/testing:service_proxy_testing",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/future:future_utility",
        "//trpc/proto/testing:cc_helloworld_proto",
        "//trpc/proto/testing:helloworld_fbs",
        "//trpc/serialization:serialization_type",
        "//trpc/stream/testing:mock_stream_provider",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/buffer:zero_copy_stream",
        "//trpc/util/flatbuffers:trpc_fbs",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_service_proxy",
    hdrs = ["trpc_service_proxy.h"],
    deps = [
        ":rpc_service_proxy",
    ],
)

cc_library(
    name = "non_rpc_service_proxy",
    hdrs = ["non_rpc_service_proxy.h"],
    deps = [
        ":service_proxy",
        "//trpc/codec:protocol",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "non_rpc_service_proxy_test",
    srcs = ["non_rpc_service_proxy_test.cc"],
    linkopts = ["-ldl"],
    deps = [
        ":non_rpc_service_proxy",
        ":service_proxy_option_setter",
        "//trpc/client:make_client_context",
        "//trpc/client/testing:service_proxy_testing",
        "//trpc/codec:client_codec_factory",
        "//trpc/codec:protocol",
        "//trpc/future:future_utility",
        "//trpc/util/buffer:noncontiguous_buffer",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "service_proxy_manager",
    srcs = ["service_proxy_manager.cc"],
    hdrs = ["service_proxy_manager.h"],
    deps = [
        ":service_proxy",
        ":service_proxy_option_setter",
        "//trpc/common/config:trpc_config",
        "//trpc/util:likely",
        "//trpc/util:net_util",
    ],
)

cc_library(
    name = "make_client_context",
    srcs = ["make_client_context.cc"],
    hdrs = ["make_client_context.h"],
    deps = [
        ":service_proxy",
        "//trpc/client:client_context",
        "//trpc/server:server_context",
        "//trpc/util:hash_util",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "make_client_context_test",
    srcs = ["make_client_context_test.cc"],
    deps = [
        ":make_client_context",
        "//trpc/client:service_proxy_option_setter",
        "//trpc/client/testing:service_proxy_testing",
        "//trpc/util:time",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_client",
    srcs = ["trpc_client.cc"],
    hdrs = ["trpc_client.h"],
    deps = [
        ":service_proxy_manager",
        "//trpc/common/config:trpc_config",
    ],
)

cc_test(
    name = "trpc_client_test",
    srcs = ["trpc_client_test.cc"],
    data = [
        "//trpc/client/testing:fiber_client.yaml",
    ],
    deps = [
        ":trpc_client",
        "//trpc/client/testing:service_proxy_testing",
        "//trpc/codec:codec_manager",
        "//trpc/naming:trpc_naming_registry",
        "//trpc/runtime",
        "@com_google_googletest//:gtest_main",
    ],
)
