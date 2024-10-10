licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mysql_statement",
    srcs = ["mysql_statement.cc"],
    hdrs = ["mysql_statement.h"],
    visibility = ["//visibility:public"],
    deps = [
        "@mysqlclient//:mysqlclient"
    ]
)

cc_library(
    name = "mysql_binder",
    hdrs = [ "mysql_binder.h"],
    deps = [
        "@mysqlclient//:mysqlclient",
    ],
)

cc_library(
    name = "mysql_results",
    srcs = ["mysql_results.cc"],
    hdrs = ["mysql_results.h"],
    deps = ["@mysqlclient//:mysqlclient"],
    visibility = ["//visibility:public"]
)

cc_library(
    name = "mysql_executor",
    srcs = ["mysql_executor.cc"],
    hdrs = ["mysql_executor.h",],
    deps = [
        "@mysqlclient//:mysqlclient",
        ":mysql_statement",
        ":mysql_results",
        ":mysql_binder",
        "@trpc_cpp//trpc/util:time",
        "@trpc_cpp//trpc/util:ref_ptr",
        "//trpc/util/log:logging",
        "//trpc/util:string_util"
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "mysql_executor_test",
    srcs = ["mysql_executor_test.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":mysql_executor",
        "//trpc/util:random",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ]
)