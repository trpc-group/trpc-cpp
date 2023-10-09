# Description: trpc-cpp.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rpcz_filter_index",
    hdrs = ["rpcz_filter_index.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   "//trpc/rpcz:span",
                   "//trpc/util:unique_id",
               ],
               "//conditions:default": [],
           }),
)

cc_library(
    name = "client_filter",
    srcs = ["client_filter.cc"],
    hdrs = ["client_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   ":rpcz_filter_index",
                   "//trpc/filter",
                   "//trpc/filter:filter_point",
                   "//trpc/rpcz",
                   "//trpc/rpcz:collector",
                   "//trpc/rpcz:span",
                   "//trpc/rpcz/util:sampler",
                   "//trpc/server:server_context",
                   "//trpc/util:time",
               ],
               "//conditions:default": [],
           }),
)

cc_test(
    name = "client_filter_test",
    srcs = ["client_filter_test.cc"],
    data = [
        "//trpc/server/testing:merge_server.yaml",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":client_filter",
        ":rpcz_filter_index",
        "//trpc/client:client_context",
        "//trpc/client:make_client_context",
        "//trpc/client:service_proxy_option_setter",
        "//trpc/client/testing:service_proxy_testing",
        "//trpc/codec:codec_manager",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/common/config:trpc_config",
        "//trpc/filter:filter_point",
        "//trpc/filter:trpc_filter",
        "//trpc/rpcz",
        "//trpc/rpcz/util:rpcz_fixture",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime:separate_runtime",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server:server_context",
        "//trpc/server/testing:service_adapter_testing",
        "//trpc/transport/common:transport_message",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_filter",
    srcs = ["server_filter.cc"],
    hdrs = ["server_filter.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   ":rpcz_filter_index",
                   "//trpc/common/config:global_conf",
                   "//trpc/common/config:trpc_config",
                   "//trpc/filter",
                   "//trpc/filter:filter_point",
                   "//trpc/rpcz",
                   "//trpc/rpcz:collector",
                   "//trpc/rpcz/util:sampler",
                   "//trpc/server:server_context",
                   "//trpc/transport/common:transport_message",
                   "//trpc/util:time",
               ],
               "//conditions:default": [],
           }),
)

cc_test(
    name = "server_filter_test",
    srcs = ["server_filter_test.cc"],
    data = [
        "//trpc/server/testing:merge_server.yaml",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":rpcz_filter_index",
        ":server_filter",
        "//trpc/codec:codec_manager",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/common/config:trpc_config",
        "//trpc/filter:filter_point",
        "//trpc/filter:trpc_filter",
        "//trpc/rpcz/util:rpcz_fixture",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime:separate_runtime",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server:server_context",
        "//trpc/server/testing:service_adapter_testing",
        "//trpc/transport/common:transport_message",
        "@com_google_googletest//:gtest_main",
    ],
)
