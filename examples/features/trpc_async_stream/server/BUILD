licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "trpc_async_stream_server",
    srcs = ["stream_server.cc"],
    linkopts = [
        "-ldl",
    ],
    deps = [
        ":stream_service",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_library(
    name = "stream_service",
    srcs = ["stream_service.cc"],
    hdrs = ["stream_service.h"],
    deps = [
        "//examples/features/trpc_stream/server:stream_proto",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)