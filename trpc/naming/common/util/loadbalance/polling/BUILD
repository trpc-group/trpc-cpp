# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "polling_load_balance",
    srcs = ["polling_load_balance.cc"],
    hdrs = ["polling_load_balance.h"],
    deps = [
        "//trpc/naming:load_balance_factory",
        "//trpc/util/log:logging",
    ],
)
