# Description: trpc-cpp.

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "selector_domain",
    srcs = ["selector_domain.cc"],
    hdrs = ["selector_domain.h"],
    deps = [
        #"//trpc/common:plugin_class_registry",
        "//trpc/common/config:domain_naming_conf",
        "//trpc/common/config:domain_naming_conf_parser",
        "//trpc/common/config:trpc_config",
        "//trpc/util/log:logging",
        "//trpc/naming:load_balance_factory",
        "//trpc/naming:selector_factory",
        "//trpc/naming/common/util:utils_help",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/util/string:string_util",
        "//trpc/util:domain_util",
        "//trpc/util:time",
    ],
)

cc_library(
    name = "domain_selector_filter",
    hdrs = ["domain_selector_filter.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/filter",
        "//trpc/naming:selector_workflow",
    ],
)

cc_test(
    name = "selector_domain_test",
    srcs = ["selector_domain_test.cc"],
    data = ["//trpc/naming/testing:domain_test.yaml"],
    deps = [
        ":selector_domain",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/common:trpc_plugin",
        "//trpc/common/config:trpc_config",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
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
