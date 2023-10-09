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
    ],
    trpc_cpp_root_path = "",
    visibility = ["//trpc:__subpackages__"],
)

exports_files(
    [
        "greeter.fbs",
        "greeter.trpc.fb_h.txt",
        "greeter.trpc.fb_cc.txt",
        "greeter_generated_h.txt",
    ],
    visibility = ["//trpc:__subpackages__"],
)
