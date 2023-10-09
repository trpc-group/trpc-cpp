licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "http_async_upload_download_server",
    srcs = ["http_server.cc"],
    linkopts = ["-ldl"],
    deps = [
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/future:future_utility",
        "@trpc_cpp//trpc/server:http_service",
        "@trpc_cpp//trpc/server/http:http_async_stream_service",
        "@trpc_cpp//trpc/util:random",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)
