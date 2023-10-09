package(default_visibility = ["//visibility:public"])

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
    deps = [
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/coroutine:fiber",
        "@trpc_cpp//trpc/runtime:runtime",
        "@trpc_cpp//trpc/util:function",
        "@trpc_cpp//trpc/util/thread:latch",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_tencent_rapidjson//:rapidjson",
    ],
)

cc_library(
    name = "subprocess",
    srcs = ["subprocess.cc"],
    hdrs = ["subprocess.h"],
    deps = [
        "@trpc_cpp//trpc/util:function",
    ],
)

cc_library(
    name = "test_signaller",
    srcs = ["test_signaller.cc"],
    hdrs = ["test_signaller.h"],
)

load("@trpc_cpp//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "test_common_proto",
    srcs = ["test_common.proto"],
    use_trpc_plugin = True,
)

cc_library(
    name = "test_common_service",
    srcs = ["test_common_service.cc"],
    hdrs = ["test_common_service.h"],
    deps = [
        ":test_common_proto",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)
