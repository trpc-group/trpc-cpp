#
#
# Copyright (c) flatbuffers original author and authors.
# flatbuffers is licensed under the Apache 2.0 License.
# The source codes in this file based on
# https://github.com/google/flatbuffers/blob/v23.5.26/BUILD.bazel.
# This source file may have been modified by Tencent, and licensed under the Apache 2.0 License.
#
#
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

exports_files([
    "LICENSE",
    "tsconfig.json",
])

config_setting(
    name = "platform_freebsd",
    constraint_values = [
        "@platforms//os:freebsd",
    ],
)

config_setting(
    name = "platform_openbsd",
    constraint_values = [
        "@platforms//os:openbsd",
    ],
)

filegroup(
    name = "distribution",
    srcs = [
        "BUILD.bazel",
        "WORKSPACE",
        "build_defs.bzl",
        "typescript.bzl",
        "//grpc/src/compiler:distribution",
        "//reflection:distribution",
        "//src:distribution",
        "//ts:distribution",
    ] + glob([
        "include/flatbuffers/*.h",
    ]),
    visibility = ["//visibility:public"],
)

# Public flatc library to compile flatbuffer files at runtime.
cc_library(
    name = "flatbuffers",
    hdrs = ["//:public_headers"],
    linkstatic = 1,
    strip_include_prefix = "/include",
    deps = ["//src:flatbuffers"],
)

# Public C++ headers for the Flatbuffers library.
filegroup(
    name = "public_headers",
    srcs = [
        "include/flatbuffers/allocator.h",
        "include/flatbuffers/array.h",
        "include/flatbuffers/base.h",
        "include/flatbuffers/buffer.h",
        "include/flatbuffers/buffer_ref.h",
        "include/flatbuffers/code_generator.h",
        "include/flatbuffers/code_generators.h",
        "include/flatbuffers/default_allocator.h",
        "include/flatbuffers/detached_buffer.h",
        "include/flatbuffers/file_manager.h",
        "include/flatbuffers/flatbuffer_builder.h",
        "include/flatbuffers/flatbuffers.h",
        "include/flatbuffers/flex_flat_util.h",
        "include/flatbuffers/flexbuffers.h",
        "include/flatbuffers/grpc.h",
        "include/flatbuffers/hash.h",
        "include/flatbuffers/idl.h",
        "include/flatbuffers/minireflect.h",
        "include/flatbuffers/reflection.h",
        "include/flatbuffers/reflection_generated.h",
        "include/flatbuffers/registry.h",
        "include/flatbuffers/stl_emulation.h",
        "include/flatbuffers/string.h",
        "include/flatbuffers/struct.h",
        "include/flatbuffers/table.h",
        "include/flatbuffers/util.h",
        "include/flatbuffers/vector.h",
        "include/flatbuffers/vector_downward.h",
        "include/flatbuffers/verifier.h",
        "include/flatbuffers/flatc.h",
    ],
)

# Public flatc compiler library.
cc_library(
    name = "flatc_library",
    linkstatic = 1,
    deps = [
        "//src:flatc_library",
    ],
)

cc_library(
    name = "generate_cpp",
    linkstatic = 1,
    deps = [
        "//src:generate_cpp",
    ],
)

cc_library(
    name = "flatc_main",
    linkstatic = 1,
    deps = [
        "//src:flatc",
    ],
)

# Public flatc compiler.
cc_binary(
    name = "flatc",
    data = ["//reflection:reflection_fbs_schema"],
    deps = [
        "//src:flatc",
    ],
)

filegroup(
    name = "flatc_headers",
    srcs = [
        "include/flatbuffers/flatc.h",
    ],
    visibility = ["//:__subpackages__"],
)

# Library used by flatbuffer_cc_library rules.
cc_library(
    name = "runtime_cc",
    hdrs = [
        "include/flatbuffers/base.h",
        "include/flatbuffers/flatbuffers.h",
        "include/flatbuffers/flexbuffers.h",
        "include/flatbuffers/stl_emulation.h",
        "include/flatbuffers/util.h",
        "include/flatbuffers/vector.h",
        "include/flatbuffers/verifier.h",
    ],
    linkstatic = 1,
    strip_include_prefix = "/include",
)
