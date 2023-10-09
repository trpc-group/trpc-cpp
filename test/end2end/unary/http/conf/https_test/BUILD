package(default_visibility = ["//visibility:public"])

filegroup(
    name = "unit_test_resources",
    testonly = 1,
    srcs = glob(
        [
            "*.yaml",
            "*.pem",
            "ca*/server.*",
        ],
        exclude_directories = 0,
    ),
    visibility = [
        "//visibility:public",
    ],
)
