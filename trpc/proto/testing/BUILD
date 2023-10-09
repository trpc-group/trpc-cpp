load("//trpc:flatbuffers.bzl", "trpc_fbs_library")
load("//trpc:trpc.bzl", "trpc_proto_library")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

trpc_proto_library(
    name = "helloworld_proto",
    srcs = ["helloworld.proto"],
    generate_mock_code = True,
    use_trpc_plugin = True,
)

cc_library(
    name = "cc_helloworld_proto",
    deps = [
        ":helloworld_proto",
    ],
)

trpc_fbs_library(
    name = "helloworld_fbs",
    srcs = ["helloworld.fbs"],
    flatc_args = [
        "--gen-object-api",
        "--gen-compare",
        "--no-includes",
        "--gen-mutable",
        "--reflect-names",
        "--cpp-ptr-type flatbuffers::unique_ptr",
    ],
)
