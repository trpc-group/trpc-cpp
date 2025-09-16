load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

# Shared protocol classes used by both client and server codecs
cc_library(
    name = "http_sse_protocol",
    srcs = ["http_sse_protocol.cc"],
    hdrs = ["http_sse_protocol.h"],
    deps = [
        "//trpc/codec/http:http_protocol",
        "//trpc/util/http/sse:http_sse_parser",
        "//trpc/log:trpc_log",
    ],
)

# Client-side SSE codec
cc_library(
    name = "http_sse_client_codec",
    srcs = ["http_sse_client_codec.cc"],
    hdrs = ["http_sse_client_codec.h"],
    deps = [
        ":http_sse_protocol",
        "//trpc/codec/http:http_client_codec",
        "//trpc/codec/http_sse:http_sse_proto_checker",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/common:status",
        "//trpc/log:trpc_log",
    ],
)

# Server-side SSE codec
cc_library(
    name = "http_sse_server_codec",
    srcs = ["http_sse_server_codec.cc"],
    hdrs = ["http_sse_server_codec.h"],
    deps = [
        ":http_sse_protocol",
        "//trpc/codec/http:http_server_codec",
        "//trpc/codec/http_sse:http_sse_proto_checker",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/common:status",
        "//trpc/log:trpc_log",
    ],
)

# Protocol checker (shared by both client and server)
cc_library(
    name = "http_sse_proto_checker",
    srcs = ["http_sse_proto_checker_impl.cc"],
    hdrs = ["http_sse_proto_checker.h"],
    deps = [
        "//trpc/codec/http:http_client_proto_checker_impl",
        "//trpc/codec/http:http_server_proto_checker_impl",
        "//trpc/util/log:logging",
    ],
)

# Combined library for backward compatibility (includes both client and server)
cc_library(
    name = "http_sse_codec",
    deps = [
        ":http_sse_client_codec",
        ":http_sse_server_codec",
        ":http_sse_protocol",
        ":http_sse_proto_checker",
    ],
)


