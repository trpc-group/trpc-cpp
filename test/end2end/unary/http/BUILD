cc_library(
    name = "http_server",
    srcs = [
        "http_server.cc",
    ],
    hdrs = [
        "http_server.h",
    ],
    deps = [
        "//test/end2end/common:test_signaller",
        "//test/end2end/common:util",
        "//trpc/common:trpc_app",
        "@trpc_cpp//trpc/util/http:function_handlers",
        "@trpc_cpp//trpc/util/http:routes",
    ],
)

cc_test(
    name = "http_test_default_merge",
    srcs = [
        "http_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/http/conf/http_test/http_client_merge.yaml",
        "--config=test/end2end/unary/http/conf/http_test/http_server_merge.yaml",
    ],
    data = [
        "//test/end2end/unary/http/conf/http_test:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":http_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/future:future_utility",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "http_test_default_separate",
    srcs = [
        "http_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/http/conf/http_test/http_client_separate.yaml",
        "--config=test/end2end/unary/http/conf/http_test/http_server_separate.yaml",
    ],
    data = [
        "//test/end2end/unary/http/conf/http_test:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":http_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/future:future_utility",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "http_test_coroutine_fiber",
    srcs = [
        "http_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/http/conf/http_test/http_client_fiber.yaml",
        "--config=test/end2end/unary/http/conf/http_test/http_server_fiber.yaml",
    ],
    data = [
        "//test/end2end/unary/http/conf/http_test:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":http_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@com_google_googletest//:gtest",
    ],
)

load("@trpc_cpp//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "http_rpc_proto",
    srcs = ["http_rpc.proto"],
    native_proto_deps = [
        "@trpc_cpp//trpc/proto:trpc_options_proto",
    ],
    use_trpc_plugin = True,
)

cc_library(
    name = "http_rpc_server",
    srcs = [
        "http_rpc_server.cc",
    ],
    hdrs = [
        "http_rpc_server.h",
    ],
    deps = [
        ":http_rpc_proto",
        "//test/end2end/common:test_signaller",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

cc_test(
    name = "http_rpc_test_default_merge",
    srcs = [
        "http_rpc_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/http/conf/http_rpc_test/http_client_merge.yaml",
        "--config=test/end2end/unary/http/conf/http_rpc_test/http_server_merge.yaml",
    ],
    data = [
        "//test/end2end/unary/http/conf/http_rpc_test:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":http_rpc_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/future:future_utility",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "http_rpc_test_default_separate",
    srcs = [
        "http_rpc_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/http/conf/http_rpc_test/http_client_separate.yaml",
        "--config=test/end2end/unary/http/conf/http_rpc_test/http_server_separate.yaml",
    ],
    data = [
        "//test/end2end/unary/http/conf/http_rpc_test:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":http_rpc_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/future:future_utility",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "http_rpc_test_coroutine_fiber",
    srcs = [
        "http_rpc_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/http/conf/http_rpc_test/http_client_fiber.yaml",
        "--config=test/end2end/unary/http/conf/http_rpc_test/http_server_fiber.yaml",
    ],
    data = [
        "//test/end2end/unary/http/conf/http_rpc_test:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":http_rpc_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/future:future_utility",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "https_server",
    srcs = [
        "https_server.cc",
    ],
    hdrs = [
        "https_server.h",
    ],
    deps = [
        ":http_rpc_proto",
        "//test/end2end/common:test_common_proto",
        "//test/end2end/common:test_common_service",
        "//test/end2end/common:test_signaller",
        "//test/end2end/common:util",
        "//trpc/common:trpc_app",
        "//trpc/util/http:function_handlers",
        "//trpc/util/http:routes",
    ],
)

cc_test(
    name = "https_test_default_merge",
    srcs = [
        "https_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/http/conf/https_test/http_client_merge.yaml",
        "--config=test/end2end/unary/http/conf/https_test/http_server_merge.yaml",
    ],
    data = [
        "//test/end2end/unary/http/conf/https_test:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":https_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/future:future_utility",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "https_test_default_separate",
    srcs = [
        "https_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/http/conf/https_test/http_client_separate.yaml",
        "--config=test/end2end/unary/http/conf/https_test/http_server_separate.yaml",
    ],
    data = [
        "//test/end2end/unary/http/conf/https_test:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":https_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/future:future_utility",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "https_test_coroutine_fiber",
    srcs = [
        "https_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/http/conf/https_test/http_client_fiber.yaml",
        "--config=test/end2end/unary/http/conf/https_test/http_server_fiber.yaml",
    ],
    data = [
        "//test/end2end/unary/http/conf/https_test:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":https_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/future:future_utility",
        "@com_google_googletest//:gtest",
    ],
)
