licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "overload_control_defs",
    hdrs = ["overload_control_defs.h"],
    defines = [] +
              select({
                  "//trpc:trpc_include_overload_control": ["TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL"],
                  "//conditions:default": [],
              }),
    visibility = ["//visibility:public"],
)
