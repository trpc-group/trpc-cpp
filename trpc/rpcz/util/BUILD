# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rpcz_fixture",
    hdrs = [
        "rpcz_fixture.h",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   "//trpc/codec/trpc/testing:trpc_protocol_testing",
                   "//trpc/proto/testing:cc_helloworld_proto",
                   "//trpc/rpcz:span",
                   "//trpc/server/rpc:rpc_service_impl",
                   "//trpc/server/testing:server_context_testing",
                   "//trpc/transport/server:server_transport_message",
                   "//trpc/util:time",
                   "//trpc/util/object_pool:object_pool_ptr",
               ],
               "//conditions:default": [],
           }),
)

cc_library(
    name = "link_list",
    hdrs = ["link_list.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
)

cc_test(
    name = "link_list_test",
    srcs = ["link_list_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":link_list",
        "//trpc/rpcz:span",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "block_group",
    hdrs = ["block_group.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   "//trpc/util:align",
                   "//trpc/util:check",
                   "//trpc/util:likely",
                   "//trpc/util/log:logging",
               ],
               "//conditions:default": [],
           }),
)

cc_test(
    name = "block_group_test",
    srcs = ["block_group_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":block_group",
        ":combiner",
        ":reducer",
        "//trpc/rpcz:span",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "combiner",
    hdrs = ["combiner.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   ":block_group",
                   ":link_list",
                   "//trpc/util/log:logging",
               ],
               "//conditions:default": [],
           }),
)

cc_test(
    name = "combiner_test",
    srcs = ["combiner_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":block_group",
        ":combiner",
        ":reducer",
        "//trpc/rpcz:collector",
        "//trpc/rpcz:span",
        "//trpc/rpcz/util:rpcz_fixture",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "reducer",
    hdrs = ["reducer.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   ":combiner",
                   "//trpc/rpcz:span",
                   "//trpc/util:likely",
                   "//trpc/util/log:logging",
               ],
               "//conditions:default": [],
           }),
)

cc_test(
    name = "reducer_test",
    srcs = ["reducer_test.cc"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":reducer",
        "//trpc/rpcz:span",
        "//trpc/rpcz/util:rpcz_fixture",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "sampler",
    srcs = ["sampler.cc"],
    hdrs = ["sampler.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [] +
           select({
               "//trpc:trpc_include_rpcz": [
                   "//trpc/common/config:trpc_config",
               ],
               "//conditions:default": [],
           }),
)

cc_test(
    name = "sampler_test",
    srcs = ["sampler_test.cc"],
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
        ":sampler",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest_main",
    ],
)
