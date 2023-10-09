load("//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "tvar_proto",
    srcs = ["tvar.proto"],
    use_trpc_plugin = True,
)

cc_library(
    name = "tvar_server",
    srcs = [
        "tvar_server.cc",
    ],
    hdrs = [
        "tvar_server.h",
    ],
    deps = [
        ":tvar_proto",
        "@trpc_cpp//test/end2end/common:test_signaller",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/tvar",
    ],
)

cc_library(
    name = "tvar_fixture",
    srcs = [
        "tvar_fixture.cc",
    ],
    hdrs = [
        "tvar_fixture.h",
    ],
    deps = [
        ":tvar_server",
        "@com_google_googletest//:gtest",
        "@trpc_cpp//test/end2end/common:subprocess",
        "@trpc_cpp//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
    ],
)

cc_test(
    name = "tvar_test_merge_noseries",
    srcs = [
        "tvar_test_noseries.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/tvar/conf/tvar_client_merge_noseries.yaml",
        "--config=test/end2end/unary/tvar/conf/tvar_server_merge_noseries.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/tvar/conf:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":tvar_fixture",
    ],
)

cc_test(
    name = "tvar_test_merge",
    srcs = [
        "tvar_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/tvar/conf/tvar_client_merge.yaml",
        "--config=test/end2end/unary/tvar/conf/tvar_server_merge.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/tvar/conf:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":tvar_fixture",
    ],
)

cc_test(
    name = "tvar_test_separate_noseries",
    srcs = [
        "tvar_test_noseries.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/tvar/conf/tvar_client_separate_noseries.yaml",
        "--config=test/end2end/unary/tvar/conf/tvar_server_separate_noseries.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/tvar/conf:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":tvar_fixture",
    ],
)

cc_test(
    name = "tvar_test_separate",
    srcs = [
        "tvar_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/tvar/conf/tvar_client_separate.yaml",
        "--config=test/end2end/unary/tvar/conf/tvar_server_separate.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/tvar/conf:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":tvar_fixture",
    ],
)

cc_test(
    name = "tvar_test_fiber_noseries",
    srcs = [
        "tvar_test_noseries.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/tvar/conf/tvar_client_fiber_noseries.yaml",
        "--config=test/end2end/unary/tvar/conf/tvar_server_fiber_noseries.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/tvar/conf:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":tvar_fixture",
    ],
)

cc_test(
    name = "tvar_test_fiber",
    srcs = [
        "tvar_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/tvar/conf/tvar_client_fiber.yaml",
        "--config=test/end2end/unary/tvar/conf/tvar_server_fiber.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/tvar/conf:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":tvar_fixture",
    ],
)
