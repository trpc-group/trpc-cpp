load("//trpc:flatbuffers.bzl", "trpc_fbs_library")

package(default_visibility = ["//visibility:public"])

trpc_fbs_library(
    name = "greeter_fbs",
    srcs = ["greeter.fbs"],
    flatc_args = [
        "--gen-object-api",
        "--gen-compare",
        "--no-includes",
        "--gen-mutable",
        "--reflect-names",
        "--cpp-ptr-type flatbuffers::unique_ptr",
        "--trpc",
    ],
    deps = [],
)

trpc_fbs_library(
    name = "forward_fbs",
    srcs = ["forward.fbs"],
    flatc_args = [
        "--gen-object-api",
        "--gen-compare",
        "--gen-mutable",
        "--reflect-names",
        "--cpp-ptr-type flatbuffers::unique_ptr",
        "--trpc",
    ],
    include_paths = ["examples/features/trpc_flatbuffers/proxy"],
    includes = [
        "greeter.fbs",
    ],
    deps = [
        ":greeter_fbs",
    ],
)

cc_binary(
    name = "forwardserver",
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":forward_server",
    ],
)

cc_library(
    name = "forward_server",
    srcs = ["forward_server.cc"],
    hdrs = ["forward_server.h"],
    deps = [
        ":forward_service",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

cc_library(
    name = "forward_service",
    srcs = ["forward_service.cc"],
    hdrs = ["forward_service.h"],
    deps = [
        ":forward_fbs",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/log:trpc_log",
    ],
)
