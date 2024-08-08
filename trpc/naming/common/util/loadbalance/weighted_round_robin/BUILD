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
    ],
)
