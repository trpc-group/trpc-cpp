# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "direct_selector_conf",
    hdrs = ["direct_selector_conf.h"],
    deps = [
        "//trpc/naming/common/util/circuit_break:circuit_breaker_config",
    ],
)

cc_library(
    name = "direct_selector_conf_parser",
    hdrs = ["direct_selector_conf_parser.h"],
    deps = [
        "//trpc/naming/direct:direct_selector_conf",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
    name = "direct_selector_conf_test",
    srcs = ["direct_selector_conf_test.cc"],
    deps = [
        ":direct_selector_conf",
        ":direct_selector_conf_parser",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "selector_direct",
    srcs = ["selector_direct.cc"],
    hdrs = ["selector_direct.h"],
    deps = [
        ":direct_selector_conf",
        ":direct_selector_conf_parser",
        "//trpc/common/config:trpc_config",
        "//trpc/naming:load_balance_factory",
        "//trpc/naming:selector_factory",
        "//trpc/naming/common/util:utils_help",
        "//trpc/naming/common/util/circuit_break:circuit_break_whitelist",
        "//trpc/naming/common/util/circuit_break:circuit_breaker",
        "//trpc/naming/common/util/circuit_break:circuit_breaker_creator_factory",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
        "//trpc/util:string_util",
        "//trpc/util:time",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "direct_selector_filter",
    hdrs = ["direct_selector_filter.h"],
    srcs = ["direct_selector_filter.cc"],
    deps = [
        ":direct_selector_conf",
        ":direct_selector_conf_parser",
        "//trpc/filter",
        "//trpc/naming:selector_workflow",
    ],
)

cc_test(
    name = "selector_direct_test",
    srcs = ["selector_direct_test.cc"],
    data = [
        "//trpc/naming/testing:direct_selector_test.yaml",
    ],
    deps = [
        ":direct_selector_conf",
        ":selector_direct",
        "//trpc/common:trpc_plugin",
        "//trpc/common/config:trpc_config",
        "//trpc/naming/common/util/circuit_break:circuit_breaker_creator_factory",
        "//trpc/naming/common/util/circuit_break:default_circuit_breaker",
        "//trpc/naming/common/util/circuit_break:default_circuit_breaker_config",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
        "//trpc/util:time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "direct_selector_filter_test",
    srcs = ["direct_selector_filter_test.cc"],
    deps = [
        ":direct_selector_filter",
        "//trpc/common:trpc_plugin",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/naming/common:common_defs",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
