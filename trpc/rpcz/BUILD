# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "span",
    srcs = ["span.cc"],
    hdrs = ["span.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }) +
              select({
                  "//trpc:trpc_disabled_objectpool": ["TRPC_DISABLED_OBJECTPOOL"],
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = ["//trpc/util/object_pool"] +
           select({
               "//trpc:trpc_include_rpcz": [
                   "//trpc/rpcz/util:link_list",
                   "//trpc/util:time",
                   "//trpc/util/log:logging",
                   "@com_github_open_source_parsers_jsoncpp//:jsoncpp",
               ],
               "//conditions:default": [],
           }),
)

cc_test(
    name = "span_test",
    srcs = ["span_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":collector",
        ":span",
        "//trpc/rpcz/util:rpcz_fixture",
        "//trpc/util/object_pool:object_pool_ptr",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "collector",
    srcs = ["collector.cc"],
    hdrs = ["collector.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   ":span",
                   "//trpc/common/config:config_helper",
                   "//trpc/common/config:global_conf",
                   "//trpc/common/config:trpc_config",
                   "//trpc/rpcz/filter:rpcz_filter_index",
                   "//trpc/rpcz/util:link_list",
                   "//trpc/rpcz/util:reducer",
                   "//trpc/rpcz/util:sampler",
                   "//trpc/runtime/common:periphery_task_scheduler",
                   "//trpc/util:time",
                   "//trpc/util/log:logging",
                   "//trpc/util/object_pool",
               ],
               "//conditions:default": [],
           }),
)

cc_test(
    name = "collector_test",
    srcs = ["collector_test.cc"],
    data = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   "//trpc/rpcz/testing:rpcz.yaml",
               ],
               "//conditions:default": [],
           }),
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":collector",
        ":span",
        "//trpc/common/config:trpc_config",
        "//trpc/rpcz/util:rpcz_fixture",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/util/object_pool:object_pool_ptr",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "rpcz",
    srcs = ["rpcz.cc"],
    hdrs = ["rpcz.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   ":span",
                   "//trpc/client:client_context",
                   "//trpc/common/config:trpc_config",
                   "//trpc/rpcz:collector",
                   "//trpc/rpcz/filter:rpcz_filter_index",
                   "//trpc/server:server_context",
                   "//trpc/util:string_util",
                   "//trpc/util:time",
                   "//trpc/util/http:request",
                   "//trpc/util/log:logging",
                   "//trpc/util/object_pool:object_pool_ptr",
               ],
               "//conditions:default": [],
           }),
)

cc_test(
    name = "rpcz_test",
    srcs = ["rpcz_test.cc"],
    data = [
        "//trpc/server/testing:merge_server.yaml",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":rpcz",
        ":span",
        "//trpc/codec:codec_manager",
        "//trpc/codec/trpc:trpc_protocol",
        "//trpc/codec/trpc:trpc_server_codec",
        "//trpc/codec/trpc/testing:trpc_protocol_testing",
        "//trpc/common/config:trpc_config",
        "//trpc/rpcz/filter:rpcz_filter_index",
        "//trpc/rpcz/util:rpcz_fixture",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime:separate_runtime",
        "//trpc/serialization:trpc_serialization",
        "//trpc/server:server_context",
        "//trpc/server/testing:service_adapter_testing",
        "//trpc/util:string_helper",
        "//trpc/util/http:request",
        "//trpc/util/object_pool:object_pool_ptr",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_rpcz",
    srcs = ["trpc_rpcz.cc"],
    hdrs = ["trpc_rpcz.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":rpcz",
        ":span",
        "//trpc/filter:filter_manager",
        "//trpc/rpcz:collector",
        "//trpc/rpcz/filter:client_filter",
        "//trpc/rpcz/filter:server_filter",
        "//trpc/server:server_context",
        "//trpc/util:string_util",
    ],
)

cc_test(
    name = "trpc_rpcz_test",
    srcs = ["trpc_rpcz_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":trpc_rpcz",
        "//trpc/filter:filter_manager",
        "//trpc/rpcz/filter:server_filter",
        "@com_google_googletest//:gtest_main",
    ],
)
