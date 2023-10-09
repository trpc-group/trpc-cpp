# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "selector_direct",
    srcs = ["selector_direct.cc"],
    hdrs = ["selector_direct.h"],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/naming:selector_factory",
        "//trpc/naming/common/util:utils_help",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
        "//trpc/util:string_util",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "direct_selector_filter",
    hdrs = ["direct_selector_filter.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/filter",
        "//trpc/naming:selector_workflow",
    ],
)

cc_test(
    name = "selector_direct_test",
    srcs = ["selector_direct_test.cc"],
    deps = [
        ":selector_direct",
        "//trpc/common:trpc_plugin",
        "//trpc/naming/common/util/loadbalance/polling:polling_load_balance",
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
