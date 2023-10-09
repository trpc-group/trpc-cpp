load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

trpc_proto_library(
    name = "helloworld_proto",
    srcs = ["helloworld.proto"],
    generate_mock_code = True,
    use_trpc_plugin = True,
)
