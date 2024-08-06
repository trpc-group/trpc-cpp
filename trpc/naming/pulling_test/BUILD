
# Description: trpc-cpp.

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)
cc_library(
    name = "selector_load_balance",
    srcs = ["selector_load_balance.cc"],
    hdrs = ["selector_load_balance.h"],
    deps = [
        #"//trpc/common:plugin_class_registry",
        "//trpc/common/config:load_balance_naming_conf",
        "//trpc/common/config:load_balance_naming_conf_parser",
        "//trpc/common/config:trpc_config",
        "//trpc/util/log:logging",
        "//trpc/naming:load_balance_factory",
        "//trpc/naming:selector_factory",
        "//trpc/naming/common/util:utils_help",
        "//trpc/naming/common/util/loadbalance/weighted_round_robin:weighted_round_robin_load_balancer",
        "//trpc/runtime/common:periphery_task_scheduler",
        "//trpc/util/string:string_util",
        "//trpc/util:domain_util",
        "//trpc/util:time",
    ],
)



cc_test(
    name = "selector_load_balance_test",
    srcs = ["selector_load_balance_test.cc"],
    data = ["//trpc/naming/testing:select_load_balance.yaml"],
    deps = [
        ":selector_load_balance",
        "//trpc/codec/trpc:trpc_client_codec",
        "//trpc/common:trpc_plugin",
        "//trpc/common/config:trpc_config",
        "//trpc/naming/common/util/loadbalance/weighted_round_robin:weighted_round_robin_load_balancer",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
