package(default_visibility = ["//visibility:public"])

cc_library(
    name = "user_rpc_filter",
    hdrs = [
        "user_rpc_filter.h",
    ],
    deps = [
        "//examples/helloworld:helloworld_proto",
        "@trpc_cpp//trpc/filter:rpc_filter",
    ],
)

cc_library(
    name = "invoke_stat_filter",
    hdrs = [
        "invoke_stat_filter.h",
    ],
    deps = [
        "@trpc_cpp//trpc/filter:filter",
        "@trpc_cpp//trpc/common/logging:trpc_logging",
    ],
)
