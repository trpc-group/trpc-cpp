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
    linkopts =["-lssl","-lcrypto"],
    deps = [
        "//trpc/naming/common/util/loadbalance/hash:MurmurHash3",
        "//trpc/naming/common/util/loadbalance/hash:City",
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
cc_library(
    name="City",
    srcs=["City.cpp"],
    hdrs=["City.h"],
    visibility=[
        "//visibility:public",
    ],
    deps=[],
)


cc_library(
    name = "selector_loadbalance_hash",
    srcs = ["selector_loadbalance_hash.cc"],
    hdrs = ["selector_loadbalance_hash.h"],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/naming:selector_factory",
        "//trpc/naming/common/util:utils_help",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
        "//trpc/util:string_util",
        "//trpc/util/log:logging",
        "//trpc/common/config:loadbalance_naming_conf",
        "//trpc/common/config:loadbalance_naming_conf_parser",
        "//trpc/common/config:trpc_config"
    ],
)

cc_test(
    name = "selector_loadbalance_hash_test",
    srcs = ["selector_loadbalance_hash_test.cc"],
    data = [
        "//trpc/naming/testing:loadbalance_test.yaml",
        "//trpc/naming/testing:consistenthash_test.yaml",
        "//trpc/naming/testing:modulohash_test.yaml",
    ],
    deps = [
        ":selector_loadbalance_hash",
        "//trpc/common:trpc_plugin",
        "//trpc/naming/common/util/loadbalance/hash:consistenthash_load_balance",
        "//trpc/naming/common/util/loadbalance/hash:modulohash_load_balance",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)