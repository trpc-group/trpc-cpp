licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mysql_statement",
    srcs = ["mysql_statement.cc"],
    hdrs = ["mysql_statement.h"],
    visibility = ["//visibility:public"]
)


cc_library(
    name = "mysql_results",
    srcs = ["mysql_results.cc"],
    hdrs = ["mysql_results.h"],
    visibility = ["//visibility:public"]
)

cc_library(
    name = "mysql_executor",
    srcs = ["mysql_executor.cc"],
    hdrs = ["mysql_executor.h", "mysql_binder.h"],
    deps = [
        ":mysql_statement",
        ":mysql_results",
        "@mysql_c_api_archive//:mysql_c_api",
        "@trpc_cpp//trpc/util:time",
        "//trpc/util:string_util"
    ],
    visibility = ["//visibility:public"],
    linkopts = ["-lmysqlclient"]
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