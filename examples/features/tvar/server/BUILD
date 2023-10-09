load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

trpc_proto_library(
    name = "tvar_proto",
    srcs = ["tvar.proto"],
    use_trpc_plugin = True,
)

cc_library(
    name = "server_lib",
    srcs = ["tvar_server.cc"],
    hdrs = ["tvar_server.h"],
    deps = [
        ":tvar_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/tvar",
    ],
)

cc_binary(
    name = "tvar_server",
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":server_lib",
    ],
)