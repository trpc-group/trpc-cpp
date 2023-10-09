package(default_visibility = ["//visibility:public"])

cc_library(
    name = "demo_protocol",
    srcs = ["demo_protocol.cc"],
    hdrs = ["demo_protocol.h"],
    deps = [
        "@trpc_cpp//trpc/codec:protocol",
    ],
)

cc_library(
    name = "demo_server_codec",
    srcs = ["demo_server_codec.cc"],
    hdrs = ["demo_server_codec.h"],
    deps = [
        ":demo_protocol",
        "@trpc_cpp//trpc/codec:server_codec",
        "@trpc_cpp//trpc/server:server_context",
    ],
)

cc_library(
    name = "demo_client_codec",
    srcs = ["demo_client_codec.cc"],
    hdrs = ["demo_client_codec.h"],
    deps = [
        ":demo_protocol",
        "@trpc_cpp//trpc/client:client_context",
        "@trpc_cpp//trpc/codec:client_codec",
    ],
)
