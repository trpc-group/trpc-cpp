licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "tracing_filter_index",
    hdrs = ["tracing_filter_index.h"],
    deps = [],
)

cc_library(
    name = "tracing",
    hdrs = ["tracing.h"],
    deps = [
        "//trpc/common:plugin",
    ],
)

cc_library(
    name = "tracing_factory",
    srcs = ["tracing_factory.cc"],
    hdrs = ["tracing_factory.h"],
    deps = [
        ":tracing",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "trpc_tracing",
    srcs = ["trpc_tracing.cc"],
    hdrs = ["trpc_tracing.h"],
    deps = [
        ":tracing",
        ":tracing_factory",
        ":tracing_filter_index",
        "//trpc/client:make_client_context",
    ],
)

cc_test(
    name = "tracing_factory_test",
    srcs = ["tracing_factory_test.cc"],
    deps = [
        ":tracing_factory",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trpc_tracing_test",
    srcs = ["trpc_tracing_test.cc"],
    deps = [
        ":trpc_tracing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
