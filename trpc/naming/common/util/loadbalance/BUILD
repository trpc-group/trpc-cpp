# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "trpc_loadbalance",
    srcs = ["trpc_loadbalance.cc"],
    hdrs = ["trpc_loadbalance.h"],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/naming:selector_factory",
        "//trpc/naming/common/util/loadbalance/weighted_round_robin:weighted_round_robin_load_balancer",
        "//trpc/naming/direct:selector_direct",
    ],
)
