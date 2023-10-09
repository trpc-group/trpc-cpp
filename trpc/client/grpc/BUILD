licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_test(
    name = "grpc_fiber_test",
    srcs = ["grpc_fiber_test.cc"],
    data = [
        "//trpc/client/grpc/testing:grpc_fiber.yaml",
    ],
    deps = [
        "//trpc/client:trpc_client",
        "//trpc/client/grpc/testing:grpc_client_call_testing",
        "//trpc/server/testing:fiber_server_testing",
        "//trpc/server/testing:greeter_service_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)