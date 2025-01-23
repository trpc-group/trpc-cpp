# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common_defs",
    hdrs = ["common_defs.h"],
    deps = [
        ":common_inc_deprecated",
        "//trpc/client:client_context",
    ],
)

cc_library(
    name = "common_inc_deprecated",
    hdrs = ["common_inc_deprecated.h"],
    defines = [] +
              select({
                  "//trpc:trpc_shared_nothing_objectpool": ["TRPC_SHARED_NOTHING_OBJECTPOOL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/codec/trpc",
        "//trpc/util:align",
        "//trpc/util/object_pool",
    ],
)

cc_library(
    name = "constants",
    hdrs = ["constants.h"],
    deps = [],
)
