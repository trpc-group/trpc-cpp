# Description: trpc-cpp.

licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

exports_files([
    "direct_selector_test.yaml",
    "domain_test.yaml",
    "domain_selector_test.yaml",
    "test.yaml",
    "test_load.yaml",
    "test_load.toml",
])

cc_library(
    name = "trpc_limiter_testing",
    hdrs = ["trpc_limiter_testing.h"],
    deps = [
        "//trpc/naming:limiter",
    ],
)

cc_library(
    name = "trpc_registry_testing",
    hdrs = ["trpc_registry_testing.h"],
    deps = [
        "//trpc/naming:registry",
    ],
)

cc_library(
    name = "trpc_selector_testing",
    hdrs = ["trpc_selector_testing.h"],
    deps = [
        "//trpc/naming:selector",
    ],
)
