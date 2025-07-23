#
#
# Copyright (C) 2016 The TensorFlow Serving Authors.
# flatbuffers is licensed under the Apache 2.0 License.
# The source codes in this file based on
# https://github.com/tensorflow/io/blob/v0.34.0/third_party/lz4.BUILD.
# This source file may have been modified by Tencent, and licensed under the Apache 2.0 License.
#
#
licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "lz4",
    srcs = [
        "lib/lz4.c",
        "lib/lz4.h",
        "lib/lz4frame.c",
        "lib/lz4frame.h",
        "lib/lz4hc.c",
        "lib/lz4hc.h",
        "lib/xxhash.h",
    ],
    hdrs = [],
    defines = [
        "XXH_PRIVATE_API",
        "LZ4LIB_VISIBILITY=",
    ],
    includes = [
        "lib",
    ],
    linkopts = [],
    textual_hdrs = [
        "lib/xxhash.c",
        "lib/lz4.c",
    ],
)
