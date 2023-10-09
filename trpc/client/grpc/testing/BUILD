licenses(["notice"])

package(default_visibility = ["//visibility:public"])

exports_files(["grpc_fiber.yaml"])


cc_library(
    name = "grpc_client_call_testing",
    srcs = ["grpc_client_call_testing.cc"],
    hdrs = ["grpc_client_call_testing.h"],
    deps = [
        "//trpc/client:make_client_context",
        "//trpc/proto/testing:helloworld_proto",
    ],
)
