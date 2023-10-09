# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "basic_traits",
    hdrs = ["basic_traits.h"],
    deps = [
        "//trpc/tvar/common:atomic_type",
        "//trpc/tvar/common:op_util",
    ],
)

cc_library(
    name = "reducer",
    hdrs = ["reducer.h"],
    deps = [
        "//trpc/tvar/basic_ops:basic_traits",
        "//trpc/tvar/common:op_util",
        "//trpc/tvar/common:tvar_group",
        "//trpc/tvar/common:write_mostly",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "reducer_test",
    srcs = ["reducer_test.cc"],
    data = ["//trpc/tvar/testing:series.yaml"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":reducer",
        "//trpc/tvar/common:op_util",
        "//trpc/tvar/common:tvar_group",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "recorder",
    hdrs = ["recorder.h"],
    deps = [
        "//trpc/tvar/basic_ops:basic_traits",
        "//trpc/tvar/common:atomic_type",
        "//trpc/tvar/common:op_util",
        "//trpc/tvar/common:tvar_group",
        "//trpc/tvar/common:write_mostly",
        "//trpc/util:likely",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "recorder_test",
    srcs = ["recorder_test.cc"],
    data = ["//trpc/tvar/testing:series.yaml"],
    deps = [
        ":recorder",
        "//trpc/tvar/common:op_util",
        "//trpc/tvar/common:tvar_group",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "status",
    hdrs = ["status.h"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/tvar/common:atomic_type",
        "//trpc/tvar/common:op_util",
        "//trpc/tvar/common:series",
        "//trpc/tvar/common:tvar_group",
        "//trpc/tvar/common:write_mostly",
    ],
)

cc_test(
    name = "status_test",
    srcs = ["status_test.cc"],
    data = ["//trpc/tvar/testing:series.yaml"],
    deps = [
        ":status",
        "//trpc/tvar/common:tvar_group",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "passive_status",
    hdrs = ["passive_status.h"],
    deps = [
        "//trpc/common/config:trpc_config",
        "//trpc/tvar/basic_ops:reducer",
        "//trpc/tvar/common:op_util",
        "//trpc/tvar/common:series",
        "//trpc/tvar/common:tvar_group",
        "//trpc/tvar/common:write_mostly",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "passive_status_test",
    srcs = ["passive_status_test.cc"],
    data = ["//trpc/tvar/testing:series.yaml"],
    deps = [
        ":passive_status",
        "//trpc/tvar/common:tvar_group",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "noseries_test",
    srcs = ["noseries_test.cc"],
    data = ["//trpc/tvar/testing:noseries.yaml"],
    deps = [
        ":passive_status",
        ":recorder",
        ":reducer",
        ":status",
        "//trpc/common/config:trpc_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
