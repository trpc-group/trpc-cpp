load("//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "fiber_proto",
    srcs = ["fiber.proto"],
    use_trpc_plugin = True,
)

cc_library(
    name = "fiber_server",
    srcs = [
        "fiber_server.cc",
    ],
    hdrs = [
        "fiber_server.h",
    ],
    deps = [
        ":fiber_proto",
        "//test/end2end/common:test_signaller",
        "//trpc/common:trpc_app",
        "//trpc/coroutine:async",
        "//trpc/coroutine:fiber_seqlock",
        "//trpc/coroutine:fiber_shared_mutex",
        "//trpc/coroutine:fiber_timer",
        "//trpc/coroutine:future",
    ],
)

cc_test(
    name = "fiber_test",
    srcs = [
        "fiber_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/fiber/conf/fiber_test/fiber_client.yaml",
        "--config=test/end2end/unary/fiber/conf/fiber_test/fiber_server.yaml",
    ],
    data = [
        "//test/end2end/unary/fiber/conf/fiber_test:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":fiber_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@com_google_googletest//:gtest",
    ],
)
