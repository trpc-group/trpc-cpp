licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "gen_hdr",
    srcs = ["gen_hdr.cc"],
    local_defines = [
        "FMT_HEADER_ONLY",
    ],
    deps = [
        "//trpc/tools/comm:utils",
        "@com_github_fmtlib_fmt//:fmtlib",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)

cc_proto_library(
    name = "cc_trpc_options_proto",
    deps = [
        "//trpc/proto:trpc_options_proto",
    ],
)

cc_proto_library(
    name = "cc_trpc_cpp_options_proto",
    deps = ["//trpc/tools/comm:trpc_cpp_options_proto"],
)

cc_library(
    name = "gen_src",
    srcs = ["gen_src.cc"],
    local_defines = [
        "FMT_HEADER_ONLY",
    ],
    deps = [
        ":cc_trpc_cpp_options_proto",
        ":cc_trpc_options_proto",
        "//trpc/tools/comm:utils",
        "@com_github_fmtlib_fmt//:fmtlib",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)

cc_library(
    name = "gen_mock",
    srcs = ["gen_mock.cc"],
    local_defines = [
        "FMT_HEADER_ONLY",
    ],
    deps = [
        "//trpc/tools/comm:utils",
        "@com_github_fmtlib_fmt//:fmtlib",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)

cc_binary(
    name = "trpc_cpp_plugin",
    srcs = ["main.cc"],
    deps = [
        ":gen_hdr",
        ":gen_mock",
        ":gen_src",
        "//trpc/tools/comm:utils",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)
