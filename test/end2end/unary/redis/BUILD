cc_library(
    name = "redis_server",
    srcs = [
        "redis_server.cc",
    ],
    hdrs = [
        "redis_server.h",
    ],
    deps = [
        "//test/end2end/common:test_signaller",
        "//test/end2end/common:util",
        "//trpc/common:trpc_app",
    ],
)

# cc_test(
#     name = "redis_client_test_default_merge",
#     srcs = [
#         "redis_client_test.cc",
#     ],
#     args = [
#         "--client_config=test/end2end/unary/redis/conf/redis_client_merge.yaml",
#         "--config=test/end2end/unary/redis/conf/redis_server.yaml",
#     ],
#     data = [
#         "//test/end2end/unary/redis/conf:unit_test_resources",
#     ],
#     linkopts = ["-lgcov"],
#     deps = [
#         ":redis_server",
#         "//test/end2end/common:subprocess",
#         "//test/end2end/common:util",
#         "//trpc/client/redis:cmdgen",
#         "//trpc/client/redis:redis_service_proxy",
#         "@com_google_googletest//:gtest",
#     ],
# )

# cc_test(
#     name = "redis_client_test_default_separte",
#     srcs = [
#         "redis_client_test.cc",
#     ],
#     args = [
#         "--client_config=test/end2end/unary/redis/conf/redis_client_separate.yaml",
#         "--config=test/end2end/unary/redis/conf/redis_server.yaml",
#     ],
#     data = [
#         "//test/end2end/unary/redis/conf:unit_test_resources",
#     ],
#     linkopts = ["-lgcov"],
#     deps = [
#         ":redis_server",
#         "//test/end2end/common:subprocess",
#         "//test/end2end/common:util",
#         "//trpc/client/redis:redis_service_proxy",
#         "@com_google_googletest//:gtest",
#     ],
# )

cc_test(
    name = "redis_client_test_coroutine_fiber",
    srcs = [
        "redis_client_test.cc",
    ],
    args = [
        "--client_config=test/end2end/unary/redis/conf/redis_client_fiber.yaml",
        "--config=test/end2end/unary/redis/conf/redis_server.yaml",
    ],
    data = [
        "//test/end2end/unary/redis/conf:unit_test_resources",
    ],
    linkopts = ["-lgcov"],
    deps = [
        ":redis_server",
        "//test/end2end/common:subprocess",
        "//test/end2end/common:util",
        "//trpc/client/redis:redis_service_proxy",
        "@com_google_googletest//:gtest",
    ],
)
