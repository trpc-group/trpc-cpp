licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "video_server",
    srcs = ["video_server.cc"],
    linkopts = ["-ldl"],
    deps = [
        ":video_stream_handler",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/server:http_service",
        "@trpc_cpp//trpc/util/http:routes",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_library(
    name = "video_stream_handler",
    srcs = ["video_stream_handler.cc"],
    hdrs = ["video_stream_handler.h"],
    deps = [
        "@trpc_cpp//trpc/util/http/stream:http_stream_handler",
    ],
)
