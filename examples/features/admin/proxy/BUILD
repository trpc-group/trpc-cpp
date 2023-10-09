load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

trpc_proto_library(
    name = "forward_proto",
    srcs = ["forward.proto"],
    use_trpc_plugin = True,
    deps = [
        "//examples/helloworld:helloworld_proto",
    ],
)

cc_library(
    name = "custom_conf",
    hdrs = ["custom_conf.h"],
    deps = [
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_binary(
    name = "forward_server",
    srcs = ["forward_server.cc"],
    deps = [
        ":custom_conf",
        ":forward_service",
        "@trpc_cpp//trpc/admin:admin_handler",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/tvar",
    ],
)

cc_library(
    name = "forward_service",
    srcs = ["forward_service.cc"],
    hdrs = ["forward_service.h"],
    deps = [
        ":forward_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/log:trpc_log",
    ],
)
