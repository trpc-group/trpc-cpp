# Description: trpc-cpp.

load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "greeter_client",
    srcs = ["greeter_client.cc"],
    hdrs = ["greeter_client.h"],
    deps = [
        "//examples/unittest:helloworld_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/log:trpc_log",
    ],
)

cc_test(
    name = "greeter_client_test",
    srcs = ["greeter_client_test.cc"],
    deps = [
        ":greeter_client",
        "//examples/unittest:helloworld_proto",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_googletest//:gtest",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/future:future_utility",
    ],
    args = [
        "--client_config=examples/unittest/client/trpc_cpp.yaml",
    ],
    data = [
        "//examples/unittest/client:trpc_cpp.yaml",
    ],
)

cc_binary(
    name = "helloworld_client",
    srcs = ["helloworld_client.cc"],
    deps = [
        ":greeter_client",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@com_github_gflags_gflags//:gflags",
    ],
)