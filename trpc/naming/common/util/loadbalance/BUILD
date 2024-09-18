# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "trpc_load_balance",
    srcs = ["trpc_load_balance.cc"],
    hdrs = ["trpc_load_balance.h"],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/naming/common/util/loadbalance/hash:consistenthash_load_balance",
        "//trpc/naming/common/util/loadbalance/hash:modulohash_load_balance",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
        "//trpc/naming/common/util/loadbalance/weighted_round_robin:weighted_round_robin_load_balancer",
    ],
)
