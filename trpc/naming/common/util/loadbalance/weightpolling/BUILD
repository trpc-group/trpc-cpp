# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "weight_polling_load_balance",
    srcs = ["weight_polling_load_balance.cc"],
    hdrs = ["weight_polling_load_balance.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/util/log:logging",
    ],
)
cc_test(
    name = "test_weight_polling_load_balance",
    srcs = ["test_weight_polling_load_balance.cc"],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/util/log:logging",
        ":weight_polling_load_balance"
    ],
)