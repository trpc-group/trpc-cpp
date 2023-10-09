licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "http_rpc_server",
    srcs = ["http_server.cc"],
    linkopts = ["-ldl"],
    deps = [
        "//examples/helloworld:greeter_service",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)
