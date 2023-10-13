load("//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "future_proto",
    srcs = ["future.proto"],
    use_trpc_plugin = True,
)

cc_library(
    name = "future_server",
    srcs = [
        "future_server.cc",
    ],
    hdrs = [
        "future_server.h",
    ],
    deps = [
        ":future_proto",
        "@trpc_cpp//test/end2end/common:test_signaller",
        "@trpc_cpp//test/end2end/common:util",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

cc_library(
    name = "future_fixture",
    srcs = [
        "future_fixture.cc",
    ],
    hdrs = [
        "future_fixture.h",
    ],
    deps = [
        ":future_server",
        "@com_google_googletest//:gtest",
        "@trpc_cpp//test/end2end/common:subprocess",
        "@trpc_cpp//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
    ],
)

cc_test(
    name = "future_test_merge",
    srcs = [
        "future_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/future/conf/future_client_merge.yaml",
        "--config=test/end2end/unary/future/conf/future_server_merge.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/future/conf:unit_test_resources",
    ],
    linkopts = [
        "-lgcov",
        "-lpthread",
    ],
    deps = [
        ":future_fixture",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/runtime:separate_runtime",
        "@trpc_cpp//trpc/util/thread:latch",
    ],
)

cc_test(
    name = "future_test_separate",
    srcs = [
        "future_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/future/conf/future_client_separate.yaml",
        "--config=test/end2end/unary/future/conf/future_server_separate.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/future/conf:unit_test_resources",
    ],
    defines = [
        "TRPC_SEPARATE_RUNTIME_TEST",
    ],
    linkopts = [
        "-lgcov",
        "-lpthread",
    ],
    deps = [
        ":future_fixture",
        "//trpc/runtime:separate_runtime",
        "//trpc/util/thread:latch",
    ],
)
