# Description: trpc-cpp.

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

exports_files([
    "test.yaml",
    "domain_test.yaml",
    "test_load.yaml",
    "test_load.toml",
    "loadbalance_test.yaml",
    "consistenthash_test.yaml",
    "modulohash_test.yaml",
])

cc_library(
    name = "trpc_limiter_testing",
    hdrs = ["trpc_limiter_testing.h"],
    deps = [
        "//trpc/naming:limiter",
    ],
)

cc_library(
    name = "trpc_registry_testing",
    hdrs = ["trpc_registry_testing.h"],
    deps = [
        "//trpc/naming:registry",
    ],
)

cc_library(
    name = "trpc_selector_testing",
    hdrs = ["trpc_selector_testing.h"],
    deps = [
        "//trpc/naming:selector",
    ],
)

cc_library(
    name = "test_selector_loadbalance",
    srcs = ["selector_loadbalance_test.cc"],
    hdrs = ["selector_loadbalance_test.h"],
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
    name = "selector_hash_test",
    srcs = ["selector_hash_test.cc"],
    data = [
        "//trpc/naming/testing:loadbalance_test.yaml",
        "//trpc/naming/testing:consistenthash_test.yaml",
        "//trpc/naming/testing:modulohash_test.yaml",
    ],
    deps = [
        ":test_selector_loadbalance",
        "//trpc/common:trpc_plugin",
        "//trpc/naming/common/util/loadbalance/hash:consistenthash_load_balance",
        "//trpc/naming/common/util/loadbalance/hash:modulohash_load_balance",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
