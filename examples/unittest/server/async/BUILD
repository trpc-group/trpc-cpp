# Description: trpc-cpp.

load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "helloworld_server",
    srcs = ["helloworld_server.cc"],
    deps = [
        ":greeter_service",
        "//examples/unittest:helloworld_proto",
        "@com_github_fmtlib_fmt//:fmtlib",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

cc_library(
    name = "greeter_service",
    srcs = ["greeter_service.cc"],
    hdrs = ["greeter_service.h"],
    deps = [
        "//examples/unittest:helloworld_proto",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/log:trpc_log",
    ],
)

cc_test(
    name = "greeter_service_test",
    srcs = ["greeter_service_test.cc"],
    deps = [
        ":greeter_service",
        "//examples/unittest:helloworld_proto",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_googletest//:gtest",
        "@trpc_cpp//trpc/codec/trpc:trpc_server_codec",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/server:make_server_context",
        "@trpc_cpp//trpc/server/testing:mock_server_transport",
        "@trpc_cpp//trpc/util/thread:latch",
    ],
)
