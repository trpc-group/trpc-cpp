# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "consistenthash_load_balance",
    srcs = ["consistenthash_load_balance.cc"],
    hdrs = ["consistenthash_load_balance.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/util/log:logging",
        "//trpc/naming/common/util/hash:hash_func",
        "//trpc/common/config:loadbalance_naming_conf",
    ],
)

cc_library(
    name = "modulohash_load_balance",
    srcs = ["modulohash_load_balance.cc"],
    hdrs = ["modulohash_load_balance.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/util/log:logging",
        "//trpc/naming/common/util/hash:hash_func",
        "//trpc/common/config:loadbalance_naming_conf",
    ],
)

