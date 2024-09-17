# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mysql_protocol",
    srcs = ["mysql_protocol.cc"],
    hdrs = ["mysql_protocol.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/codec:protocol",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "mysql_client_codec",
    srcs = ["mysql_client_codec.cc"],
    hdrs = ["mysql_client_codec.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":mysql_protocol",
        "//trpc/client:client_context",
        "//trpc/codec:client_codec",
        "//trpc/util/log:logging",
    ],
)

