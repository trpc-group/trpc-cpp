licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "https_server",
    srcs = ["https_server.cc"],
    linkopts = ["-ldl"],
    deps = [
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/server:http_service",
        "@trpc_cpp//trpc/util/http:http_handler",
        "@trpc_cpp//trpc/util/http:routes",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)
