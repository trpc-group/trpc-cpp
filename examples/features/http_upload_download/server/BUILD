licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "http_upload_download_server",
    srcs = ["http_server.cc"],
    linkopts = ["-ldl"],
    deps = [
        ":file_storage_handler",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/server:http_service",
        "@trpc_cpp//trpc/util/http:routes",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_library(
    name = "file_storage_handler",
    srcs = ["file_storage_handler.cc"],
    hdrs = ["file_storage_handler.h"],
    deps = [
        "@trpc_cpp//trpc/util/http/stream:http_stream_handler",
        "//examples/features/http_upload_download/common:file_hasher",
    ],
)
