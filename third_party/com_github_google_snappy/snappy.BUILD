#
#
# Copyright (C) 2016 The TensorFlow Serving Authors.
# flatbuffers is licensed under the Apache 2.0 License.
# The source codes in this file based on
# https://github.com/tensorflow/io/blob/v0.34.0/third_party/snappy.BUILD.
# This source file may have been modified by Tencent, and licensed under the Apache 2.0 License.
#
#
licenses(["notice"])

package(default_visibility = ["//visibility:public"])

genrule(
    name = "snappy_stubs_public_h",
    srcs = [
        "snappy-stubs-public.h.in",
    ],
    outs = [
        "snappy-stubs-public.h",
    ],
    # use: sys/uio.h.
    # use: version 1.1.8.
    cmd = "sed 's/$${HAVE_SYS_UIO_H_01}/1/g' $(<) | " +
          "sed 's/$${PROJECT_VERSION_MAJOR}/1/g' | " +
          "sed 's/$${PROJECT_VERSION_MINOR}/1/g' | " +
          "sed 's/$${PROJECT_VERSION_PATCH}/8/g' >$(@)",
    visibility = ["//visibility:private"],
)

cc_library(
    name = "snappy",
    srcs = glob(
        ["**/*.cc"],
        exclude = [
            "snappy_compress_fuzzer.cc",
            "snappy-test.cc",
            "snappy_unittest.cc",
        ],
    ),
    hdrs = glob(
        ["**/*.h"],
        exclude = [
            "snappy_compress_fuzzer.h",
            "snappy-test.h",
        ],
    ) + [
        ":snappy_stubs_public_h",
    ],
    copts = [
        "-Wno-non-virtual-dtor",
        "-Wno-unused-variable",
        "-Wno-implicit-fallthrough",
        "-Wno-unused-function",
    ],
    includes = ["."],
)
