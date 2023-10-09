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

cc_binary(
    name = "demoserver",
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":demo_server",
    ],
)

cc_library(
    name = "demo_server",
    srcs = ["demo_server.cc"],
    hdrs = ["demo_server.h"],
    deps = [
        ":demo_service",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

cc_library(
    name = "demo_service",
    srcs = ["demo_service.cc"],
    hdrs = ["demo_service.h"],
    deps = [
        ":greeter_fbs",
        "@trpc_cpp//trpc/log:trpc_log",
        "@trpc_cpp//trpc/util/flatbuffers:trpc_fbs",
    ],
)
