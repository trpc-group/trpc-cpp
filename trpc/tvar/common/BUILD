# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "op_util",
    hdrs = ["op_util.h"],
    deps = [
        "//trpc/util/log:logging",
        "@com_github_open_source_parsers_jsoncpp//:jsoncpp",
    ],
)

cc_test(
    name = "op_util_test",
    srcs = ["op_util_test.cc"],
    deps = [
        ":op_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "atomic_type",
    hdrs = ["atomic_type.h"],
    deps = [
        ":op_util",
    ],
)

cc_test(
    name = "atomic_type_test",
    srcs = ["atomic_type_test.cc"],
    deps = [
        ":atomic_type",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "macros",
    hdrs = [
        "macros.h",
    ],
)

cc_library(
    name = "series",
    hdrs = [
        "series.h",
    ],
)

cc_test(
    name = "series_test",
    srcs = ["series_test.cc"],
    deps = [
        ":op_util",
        ":series",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "fast_rand",
    srcs = [
        "fast_rand.cc",
    ],
    hdrs = [
        "fast_rand.h",
    ],
    deps = [
        ":op_util",
    ],
)

cc_library(
    name = "tvar_group",
    srcs = ["tvar_group.cc"],
    hdrs = ["tvar_group.h"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/util:deferred",
        "//trpc/util:function",
        "//trpc/util/log:logging",
        "//trpc/util/string:string_helper",
        "@com_github_open_source_parsers_jsoncpp//:jsoncpp",
    ],
)

cc_test(
    name = "tvar_group_test",
    srcs = ["tvar_group_test.cc"],
    deps = [
        ":op_util",
        ":tvar_group",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "write_mostly",
    srcs = [
        "sampler.cc",
    ],
    hdrs = [
        "sampler.h",
        "write_mostly.h",
    ],
    deps = [
        ":atomic_type",
        ":op_util",
        ":series",
        ":tvar_group",
        "//trpc/common/config:trpc_config",
        "//trpc/util:align",
        "//trpc/util:check",
        "//trpc/util/chrono:time",
        "//trpc/util/container:bounded_queue",
        "//trpc/util/log:logging",
        "//trpc/util/thread:thread_helper",
        "//trpc/util/thread:thread_local",
    ],
)

cc_test(
    name = "sampler_test",
    srcs = ["sampler_test.cc"],
    deps = [
        ":write_mostly",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "percentile",
    srcs = [
        "percentile.cc",
    ],
    hdrs = [
        "percentile.h",
    ],
    deps = [
        ":atomic_type",
        ":fast_rand",
        ":macros",
        ":op_util",
        ":write_mostly",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "percentile_test",
    srcs = ["percentile_test.cc"],
    deps = [
        ":percentile",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
