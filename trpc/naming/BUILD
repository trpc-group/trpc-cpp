# Description: trpc-cpp.

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "limiter",
    hdrs = ["limiter.h"],
    deps = [
        "//trpc/common:plugin",
        "//trpc/naming/common:common_defs",
    ],
)

cc_library(
    name = "limiter_factory",
    srcs = ["limiter_factory.cc"],
    hdrs = ["limiter_factory.h"],
    deps = [
        ":limiter",
    ],
)

cc_test(
    name = "limiter_factory_test",
    srcs = ["limiter_factory_test.cc"],
    deps = [
        ":limiter_factory",
        "//trpc/naming/testing:trpc_limiter_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "registry",
    hdrs = ["registry.h"],
    deps = [
        "//trpc/common:plugin",
        "//trpc/common/future",
        "//trpc/naming/common:common_defs",
    ],
)

cc_library(
    name = "registry_factory",
    srcs = ["registry_factory.cc"],
    hdrs = ["registry_factory.h"],
    deps = [
        ":registry",
    ],
)

cc_test(
    name = "registry_factory_test",
    srcs = ["registry_factory_test.cc"],
    deps = [
        ":registry_factory",
        "//trpc/naming/testing:trpc_registry_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "selector",
    hdrs = ["selector.h"],
    deps = [
        "//trpc/common:plugin",
        "//trpc/common/future",
        "//trpc/naming/common:common_defs",
    ],
)

cc_library(
    name = "selector_factory",
    srcs = ["selector_factory.cc"],
    hdrs = ["selector_factory.h"],
    deps = [
        ":selector",
    ],
)

cc_test(
    name = "selector_factory_test",
    srcs = ["selector_factory_test.cc"],
    deps = [
        ":selector_factory",
        "//trpc/naming/testing:trpc_selector_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "selector_workflow_test",
    srcs = ["selector_workflow_test.cc"],
    deps = [
        ":selector",
        ":selector_factory",
        ":selector_workflow",
        "//trpc/client:client_context",
        "//trpc/codec/trpc:trpc_client_codec",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "load_balance",
    hdrs = ["load_balance.h"],
    deps = [
        "//trpc/naming/common:common_defs",
    ],
)

cc_library(
    name = "load_balance_factory",
    srcs = ["load_balance_factory.cc"],
    hdrs = ["load_balance_factory.h"],
    deps = [
        ":load_balance",
    ],
)

cc_library(
    name = "trpc_naming",
    srcs = ["trpc_naming.cc"],
    hdrs = ["trpc_naming.h"],
    deps = [
        ":limiter",
        ":limiter_factory",
        ":registry",
        ":registry_factory",
        ":selector",
        ":selector_factory",
        "//trpc/common/config:trpc_config",
        "//trpc/future",
    ],
)

cc_library(
    name = "selector_workflow",
    srcs = ["selector_workflow.cc"],
    hdrs = ["selector_workflow.h"],
    deps = [
        ":selector_factory",
        "//trpc/client:client_context",
        "//trpc/common/config:trpc_config",
        "//trpc/filter",
        "//trpc/log:trpc_log",
        "//trpc/naming/common:common_defs",
        "//trpc/naming/common:constants",
        "//trpc/transport/client:retry_info_def",
        "//trpc/util:time",
        "//trpc/util/string:string_util",
    ],
)

cc_library(
    name = "trpc_naming_registry",
    srcs = ["trpc_naming_registry.cc"],
    hdrs = ["trpc_naming_registry.h"],
    deps = [
        ":limiter",
        ":limiter_factory",
        ":registry",
        ":registry_factory",
        ":selector",
        ":selector_factory",
        "//trpc/filter:filter_manager",
        "//trpc/naming:load_balance_factory",
        "//trpc/naming/direct:direct_selector_filter",
        "//trpc/naming/direct:selector_direct",
        "//trpc/naming/domain:domain_selector_filter",
        "//trpc/naming/domain:selector_domain",
    ],
)
