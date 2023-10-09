filegroup(
    name = "unit_test_resources",
    testonly = 1,
    srcs = glob(
        [
            "async_test/*.yaml",
            "sync_test/*.yaml",
        ],
        exclude_directories = 0,
    ),
    visibility = [
        "//visibility:public",
    ],
)
