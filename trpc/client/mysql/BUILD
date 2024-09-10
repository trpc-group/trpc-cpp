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
        ":mysql_statement",
        "@mysql_c_api_archive//:mysql_c_api",
        "@trpc_cpp//trpc/util:time",
    ],
    visibility = ["//visibility:public"],
    linkopts = ["-lmysqlclient"]
)

cc_library(
    name = "mysql_executor_pool",
    srcs = ["mysql_executor_pool.cc"],
    hdrs = ["mysql_executor_pool.h"],
    deps = [
        ":mysql_executor"
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "mysql_service_proxy",
    srcs = ["mysql_service_proxy.cc"],
    hdrs = ["mysql_service_proxy.h"],
    deps = [
        ":mysql_executor",
        "//trpc/util/thread:thread_pool",
        "//trpc/util/thread:latch",
        "//trpc/coroutine:fiber",
        "//trpc/client:service_proxy",
        "//trpc/common/logging:trpc_logging"
    ],
    visibility = ["//visibility:public"]
)

cc_test(
    name = "mysql_executor_test",
    srcs = ["mysql_executor_test.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":mysql_executor",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ]
)

cc_test(
    name = "mysql_service_proxy_test",
    srcs = ["mysql_service_proxy_test.cc"],
        visibility = ["//visibility:public"],
    deps = [
        ":mysql_service_proxy",
        "//trpc/client:make_client_context",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ]
)

cc_test(
    name = "mysql_executor_pool_test",
    srcs = ["mysql_executor_pool_test.cc"],
    visibility = ["//visibility:public"],
    deps = [
        ":mysql_executor",
        ":mysql_executor_pool",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)