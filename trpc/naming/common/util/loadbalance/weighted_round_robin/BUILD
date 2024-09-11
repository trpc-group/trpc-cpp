# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "weighted_round_robin_load_balancer",
    srcs = ["weighted_round_robin_load_balancer.cc"],
    hdrs = ["weighted_round_robin_load_balancer.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/util/log:logging",
        "//trpc/common/config:trpc_config",
    ],
)
cc_test(
    name = "weighted_round_robin_load_balancer_test",
    srcs = ["weighted_round_robin_load_balancer_test.cc"],
    deps = [
        ":weighted_round_robin_load_balancer",
        "//trpc/naming/direct:selector_direct",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/common/config:trpc_config",
        "//trpc/runtime/common:periphery_task_scheduler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
