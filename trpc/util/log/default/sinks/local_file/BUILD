licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hourly_file_sink",
    hdrs = ["hourly_file_sink.h"],
    deps = [
        "@com_github_gabime_spdlog//:spdlog",
    ],
)

cc_test(
    name = "hourly_file_sink_test",
    srcs = ["hourly_file_sink_test.cc"],
    deps = [
        ":hourly_file_sink",
        ":local_file_sink",
        "//trpc/util/log/default:default_log",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "local_file_sink",
    srcs = ["local_file_sink.cc"],
    hdrs = ["local_file_sink.h"],
    deps = [
        ":file_helper",
        ":hourly_file_sink",
        "//trpc/common/config:default_log_conf",
        "//trpc/common/config:local_file_sink_conf",
        "//trpc/util:ref_ptr",
        "@com_github_gabime_spdlog//:spdlog",
    ],
)

cc_test(
    name = "local_file_sink_test",
    srcs = ["local_file_sink_test.cc"],
    deps = [
        ":local_file_sink",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "file_helper",
    srcs = ["file_helper.cc"],
    hdrs = ["file_helper.h"],
)

cc_test(
    name = "file_helper_test",
    srcs = ["file_helper_test.cc"],
    deps = [
        ":file_helper",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
