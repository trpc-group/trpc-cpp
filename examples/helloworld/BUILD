# Description: trpc-cpp.

load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "helloworld_svr",
    srcs = ["helloworld_server.cc"],
    deps = [
        ":greeter_service",
        ":helloworld_proto",
        "@com_github_fmtlib_fmt//:fmtlib",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

trpc_proto_library(
    name = "helloworld_proto",
    srcs = ["helloworld.proto"],
    generate_mock_code = True,
    use_trpc_plugin = True,
)

cc_library(
    name = "greeter_service",
    srcs = ["greeter_service.cc"],
    hdrs = ["greeter_service.h"],
    deps = [
        ":helloworld_proto",
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
        ":helloworld_proto",
        "@com_google_googletest//:gtest_main",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/server:make_server_context",
        "@trpc_cpp//trpc/server:trpc_server",
    ],
)
