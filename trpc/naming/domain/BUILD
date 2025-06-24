# Description: trpc-cpp.

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "domain_selector_conf",
    hdrs = [
        "domain_selector_conf.h",
        "domain_selector_conf_parser.h",
    ],
    srcs = ["domain_selector_conf.cc"],
    deps = [
        "//trpc/util/log:logging",
        "//trpc/naming/common/util/circuit_break:circuit_breaker_config",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
    name = "domain_selector_conf_test",
    srcs = ["domain_selector_conf_test.cc"],
    deps = [
        ":domain_selector_conf",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "selector_domain",
    srcs = ["selector_domain.cc"],
    hdrs = ["selector_domain.h"],
    deps = [
        ":domain_selector_conf",
        "//trpc/common/config:trpc_config",
        "//trpc/util/log:logging",
        "//trpc/naming:load_balance_factory",
        "//trpc/naming:selector_factory",
        "//trpc/naming/common/util:utils_help",
        "//trpc/naming/common/util/circuit_break:circuit_break_whitelist",
        "//trpc/naming/common/util/circuit_break:circuit_breaker",
        "//trpc/naming/common/util/circuit_break:circuit_breaker_creator_factory",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/util/string:string_util",
        "//trpc/util:domain_util",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "domain_selector_filter",
    srcs = ["domain_selector_filter.cc"],
    hdrs = ["domain_selector_filter.h"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/filter",
        "//trpc/naming:selector_workflow",
        "//trpc/naming/domain:domain_selector_conf",
    ],
)

cc_test(
    name = "selector_domain_test",
    srcs = ["selector_domain_test.cc"],
    data = [
        "//trpc/naming/testing:domain_test.yaml",
        "//trpc/naming/testing:domain_selector_test.yaml",
    ],
    deps = [
        ":selector_domain",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/common:trpc_plugin",
        "//trpc/common/config:trpc_config",
        "//trpc/naming/common/util/circuit_break:circuit_breaker_creator_factory",
        "//trpc/naming/common/util/circuit_break:default_circuit_breaker",
        "//trpc/naming/common/util/circuit_break:default_circuit_breaker_config",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
        "//trpc/util:domain_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "domain_selector_filter_test",
    srcs = ["domain_selector_filter_test.cc"],
    deps = [
        ":domain_selector_filter",
        "//trpc/common:trpc_plugin",
        "//trpc/filter",
        "//trpc/filter:filter_manager",
        "//trpc/naming/common:common_defs",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
