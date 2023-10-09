load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

trpc_proto_library(
    name = "rpcz_proto",
    srcs = ["rpcz.proto"],
    use_trpc_plugin = True,
    deps = [
        "//examples/helloworld:helloworld_proto",
    ],
)

cc_library(
    name = "proxy_lib",
    srcs = ["rpcz_proxy.cc"],
    hdrs = ["rpcz_proxy.h"],
    deps = [
        ":rpcz_proto",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/future:future_utility",
        "@trpc_cpp//trpc/rpcz:trpc_rpcz",
    ],
)

cc_binary(
    name = "rpcz_proxy",
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":proxy_lib",
    ],
)
