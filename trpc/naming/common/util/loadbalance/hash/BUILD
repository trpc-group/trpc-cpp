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
        "//trpc/naming/common/util/loadbalance/hash:hash_func",
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
        "//trpc/naming/common/util/loadbalance/hash:hash_func",
        "//trpc/common/config:loadbalance_naming_conf",
    ],
)

cc_library(
    name = "hash_func",
    hdrs = ["hash_func.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming/common/util/loadbalance/hash:MurmurHash3"
    ],
)

cc_library(
    name="MurmurHash3",
    srcs=["MurmurHash3.cpp"],
    hdrs=["MurmurHash3.h"],
    visibility=[
        "//visibility:public",
    ],
    deps=[],
)
