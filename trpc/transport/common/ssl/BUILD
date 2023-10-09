licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "core",
    hdrs = ["core.h"],
)

cc_library(
    name = "errno",
    hdrs = ["errno.h"],
)

cc_library(
    name = "random",
    hdrs = ["random.h"],
)

cc_test(
    name = "random_test",
    srcs = ["random_test.cc"],
    deps = [
        ":random",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "ssl",
    srcs = ["ssl.cc"],
    hdrs = ["ssl.h"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/util:ref_ptr",
        "//trpc/util/log:logging",
    ] + select({
        "//trpc:include_ssl": [
            ":core",
            ":errno",
            "@com_github_openssl_openssl//:libcrypto",
            "@com_github_openssl_openssl//:libssl",
        ],
        "//trpc:trpc_include_ssl": [
            ":core",
            ":errno",
            "@com_github_openssl_openssl//:libcrypto",
            "@com_github_openssl_openssl//:libssl",
        ],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "ssl_test",
    srcs = ["ssl_test.cc"],
    data = ["//trpc/transport/common/ssl:unit_test_resourses"],
    defines = [] +
              select({
                  "//trpc:include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//trpc:trpc_include_ssl": ["TRPC_BUILD_INCLUDE_SSL"],
                  "//conditions:default": [],
              }),
    deps = [
        "//trpc/server/testing:http_client_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ] + select({
        "//conditions:default": [],
        "//trpc:include_ssl": [
            ":core",
            ":random",
            ":ssl",
        ],
        "//trpc:trpc_include_ssl": [
            ":core",
            ":random",
            ":ssl",
        ],
    }),
)

filegroup(
    name = "unit_test_resourses",
    testonly = 1,
    srcs = glob(
        ["cert/**"],
        exclude_directories = 0,
    ),
)
