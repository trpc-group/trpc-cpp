# Description: trpc-cpp.

licenses(["notice"])

cc_library(
    name = "default_config",
    srcs = ["default_config.cc"],
    hdrs = ["default_config.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/common/config:default_value",
        "//trpc/config:codec",
        "//trpc/config:provider",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "default_config_test",
    srcs = ["default_config_test.cc"],
    deps = [
        ":default_config",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "loader",
    srcs = ["loader.cc"],
    hdrs = ["loader.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":default_config",
        "//trpc/config:codec_factory",
        "//trpc/config:provider_factory",
        "//trpc/util:ref_ptr",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "loader_test",
    srcs = ["loader_test.cc"],
    copts = [
        "-DGTEST_HAS_DEATH_TEST=1",  # Add this option to enable death tests
    ],
    deps = [
        ":default_config",
        ":loader",
        "//trpc/config:config_factory",
        "//trpc/config:provider_factory",
        "//trpc/config/testing:codec_plugin_testing",
        "//trpc/config/testing:provider_plugin_testing",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
