# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "tvar",
    hdrs = ["tvar.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/tvar/basic_ops:passive_status",
        "//trpc/tvar/basic_ops:recorder",
        "//trpc/tvar/basic_ops:reducer",
        "//trpc/tvar/basic_ops:status",
        "//trpc/tvar/common:tvar_group",
        "//trpc/tvar/compound_ops:latency_recorder",
        "//trpc/tvar/compound_ops:window",
    ],
)
