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

cc_library(
    name = "server_overload_controller",
    hdrs = ["server_overload_controller.h"],
    deps = [
        "//trpc/server:server_context",
    ],
)

cc_library(
    name = "server_overload_controller_factory",
    hdrs = ["server_overload_controller_factory.h"],
    deps = [
        ":server_overload_controller",
        "//trpc/overload_control/common:overload_control_factory",
    ],
)

cc_test(
    name = "server_overload_controller_factory_test",
    srcs = ["server_overload_controller_factory_test.cc"],
    deps = [
        ":server_overload_controller_factory",
        "//trpc/overload_control/testing:overload_control_testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "trpc_overload_control",
    srcs = ["trpc_overload_control.cc"],
    hdrs = ["trpc_overload_control.h"],
    deps = [
        ":server_overload_controller_factory",
        "//trpc/filter:filter_manager",
    ],
)
