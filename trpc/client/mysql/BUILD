licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mysql_statement",
    srcs = ["mysql_statement.cc"],
    hdrs = ["mysql_statement.h"],
    visibility = ["//visibility:public"]
)

cc_library(
    name = "mysql_executor",
    srcs = ["mysql_executor.cc"],
    hdrs = ["mysql_executor.h", "mysql_binder.h"],
    deps = [
        ":mysql_statement"
    ],
    visibility = ["//visibility:public"],
    linkopts = [
        "-L/usr/lib/x86_64-linux-gnu -lmysqlclient -lzstd -lssl -lcrypto -lresolv -lm -pthread",
    ]
)

cc_binary(
    name = "mysql_executor_test",
    srcs = ["mysql_executor_test.cc"],
    deps = [
        ":mysql_executor",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ]
)