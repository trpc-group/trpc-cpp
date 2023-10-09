load("//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "real_server_proto",
    srcs = ["real_server.proto"],
    use_trpc_plugin = True,
)

trpc_proto_library(
    name = "rpcz_proto",
    srcs = ["rpcz.proto"],
    use_trpc_plugin = True,
)

cc_library(
    name = "real_server",
    srcs = [
        "real_server.cc",
    ],
    hdrs = [
        "real_server.h",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":real_server_proto",
        "@trpc_cpp//test/end2end/common:test_signaller",
        "@trpc_cpp//trpc/common:trpc_app",
    ],
)

cc_library(
    name = "rpcz_server",
    srcs = [
        "rpcz_server.cc",
    ],
    hdrs = [
        "rpcz_server.h",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":real_server_proto",
        ":rpcz_proto",
        "@trpc_cpp//test/end2end/common:test_signaller",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/rpcz:trpc_rpcz",
        "@trpc_cpp//trpc/runtime",
    ],
)

cc_library(
    name = "rpcz_fixture",
    srcs = [
        "rpcz_fixture.cc",
    ],
    hdrs = [
        "rpcz_fixture.h",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    deps = [
        ":real_server",
        ":rpcz_server",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@trpc_cpp//test/end2end/common:subprocess",
        "@trpc_cpp//test/end2end/common:util",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
    ],
)

cc_test(
    name = "rpcz_test_fiber",
    srcs = [
        "rpcz_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/rpcz/conf/rpcz_client_fiber.yaml",
        "--config=test/end2end/unary/rpcz/conf/rpcz_proxy_fiber.yaml",
        "--config=test/end2end/unary/rpcz/conf/rpcz_real_server_fiber.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/rpcz/conf:unit_test_resources",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/util:function",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    linkopts = [
        "-lgcov",
        "-lpthread",
    ],
    deps = [
        ":rpcz_fixture",
    ],
)

cc_test(
    name = "rpcz_test_merge",
    srcs = [
        "rpcz_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/rpcz/conf/rpcz_client_merge.yaml",
        "--config=test/end2end/unary/rpcz/conf/rpcz_proxy_merge.yaml",
        "--config=test/end2end/unary/rpcz/conf/rpcz_real_server_merge.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/rpcz/conf:unit_test_resources",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    linkopts = [
        "-lgcov",
        "-lpthread",
    ],
    deps = [
        ":rpcz_fixture",
    ],
)

cc_test(
    name = "rpcz_test_separate",
    srcs = [
        "rpcz_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/rpcz/conf/rpcz_client_separate.yaml",
        "--config=test/end2end/unary/rpcz/conf/rpcz_proxy_separate.yaml",
        "--config=test/end2end/unary/rpcz/conf/rpcz_real_server_separate.yaml",
    ],
    data = [
        "@trpc_cpp//test/end2end/unary/rpcz/conf:unit_test_resources",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
    ],
    defines = [] +
              select({
                  "//trpc:trpc_include_rpcz": ["TRPC_BUILD_INCLUDE_RPCZ"],
                  "//conditions:default": [],
              }),
    linkopts = [
        "-lgcov",
        "-lpthread",
    ],
    deps = [
        ":rpcz_fixture",
    ],
)
