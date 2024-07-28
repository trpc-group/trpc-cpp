licenses(["notice"])

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "config_helper",
    srcs = ["config_helper.cc"],
    hdrs = ["config_helper.h"],
    deps = [
        ":yaml_parser",
    ],
)

cc_library(
    name = "yaml_parser",
    hdrs = ["yaml_parser.h"],
    deps = [
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "client_conf",
    srcs = ["client_conf.cc"],
    hdrs = ["client_conf.h"],
    deps = [
        ":default_value",
        ":redis_client_conf",
        ":retry_conf",
        ":ssl_conf",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "client_conf_parser",
    hdrs = ["client_conf_parser.h"],
    deps = [
        ":client_conf",
        ":redis_client_conf_parser",
        ":retry_conf_parser",
        ":ssl_conf_parser",
    ],
)

cc_library(
    name = "default_log_conf_h",
    hdrs = ["default_log_conf.h"],
)

cc_library(
    name = "default_log_conf",
    srcs = ["default_log_conf.cc"],
    deps = [
        ":config_helper",
        ":default_log_conf_h",
        ":default_log_conf_parser",
    ],
)

cc_library(
    name = "default_log_conf_parser",
    srcs = ["default_log_conf_parser.cc"],
    hdrs = ["default_log_conf_parser.h"],
    deps = [
        ":default_value",
        ":config_helper",
        ":default_log_conf_h",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "default_value",
    hdrs = ["default_value.h"],
    deps = [],
)

cc_library(
    name = "local_file_sink_conf",
    srcs = ["local_file_sink_conf.cc"],
    hdrs = [
        "local_file_sink_conf.h",
        "local_file_sink_conf_parser.h",
    ],
    deps = [
        ":config_helper",
        ":default_log_conf",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "stdout_sink_conf",
    srcs = ["stdout_sink_conf.cc"],
    hdrs = [
        "stdout_sink_conf.h",
        "stdout_sink_conf_parser.h",
    ],
    deps = [
        ":config_helper",
        ":default_log_conf",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "redis_client_conf",
    srcs = ["redis_client_conf.cc"],
    hdrs = ["redis_client_conf.h"],
    deps = [
        "//trpc/util/log:logging",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "redis_client_conf_parser",
    hdrs = ["redis_client_conf_parser.h"],
    deps = [
        ":redis_client_conf",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "retry_conf",
    srcs = ["retry_conf.cc"],
    hdrs = ["retry_conf.h"],
    deps = [
        "//trpc/util/log:logging",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "retry_conf_parser",
    hdrs = ["retry_conf_parser.h"],
    deps = [
        ":retry_conf",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
    name = "retry_conf_test",
    srcs = ["retry_conf_test.cc"],
    deps = [
        ":retry_conf",
        ":retry_conf_parser",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "server_conf",
    srcs = ["server_conf.cc"],
    hdrs = ["server_conf.h"],
    defines = [] + select({
        "//trpc:trpc_disable_reuseport": ["TRPC_DISABLE_REUSEPORT"],
        "//conditions:default": [],
    }),
    deps = [
        ":ssl_conf",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "server_conf_parser",
    hdrs = ["server_conf_parser.h"],
    defines = [] + select({
        "//trpc:trpc_disable_reuseport": ["TRPC_DISABLE_REUSEPORT"],
        "//conditions:default": [],
    }),
    deps = [
        ":server_conf",
        ":ssl_conf_parser",
        "//trpc/util:net_util",
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "ssl_conf",
    srcs = ["ssl_conf.cc"],
    hdrs = ["ssl_conf.h"],
    deps = [
        "//trpc/util/log:logging",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_test(
    name = "ssl_conf_test",
    srcs = ["ssl_conf_test.cc"],
    deps = [
        ":ssl_conf",
        ":ssl_conf_parser",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "ssl_conf_parser",
    hdrs = ["ssl_conf_parser.h"],
    deps = [
        ":ssl_conf",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "global_conf",
    srcs = ["global_conf.cc"],
    hdrs = ["global_conf.h"],
    deps = [
        "//trpc/util/log:logging",
    ],
)

cc_library(
    name = "global_conf_parser",
    hdrs = ["global_conf_parser.h"],
    deps = [
        ":global_conf",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "trpc_config",
    srcs = ["trpc_config.cc"],
    hdrs = ["trpc_config.h"],
    deps = [
        ":client_conf",
        ":client_conf_parser",
        ":config_helper",
        ":global_conf",
        ":global_conf_parser",
        ":server_conf",
        ":server_conf_parser",
        "//trpc/util/chrono",
        "//trpc/util/log:logging",
    ],
)

cc_test(
    name = "client_conf_test",
    srcs = ["client_conf_test.cc"],
    deps = [
        ":client_conf",
        ":client_conf_parser",
        ":yaml_parser",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "server_conf_test",
    srcs = ["server_conf_test.cc"],
    deps = [
        ":server_conf",
        ":server_conf_parser",
        "@com_google_googletest//:gtest_main",
    ],
)

filegroup(
    name = "test_yaml_files",
    srcs = glob([
        "testing/*.yaml",
    ]),
)

cc_test(
    name = "trpc_config_test",
    srcs = ["trpc_config_test.cc"],
    data = [
        ":test_yaml_files",
    ],
    deps = [
        ":trpc_config",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "domain_naming_conf",
    srcs = ["domain_naming_conf.cc"],
    hdrs = ["domain_naming_conf.h"],
    deps = [
        "//trpc/util/log:logging",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "domain_naming_conf_parser",
    hdrs = ["domain_naming_conf_parser.h"],
    deps = [
        ":domain_naming_conf",
    ],
)

cc_test(
    name = "domain_naming_conf_test",
    srcs = ["domain_naming_conf_test.cc"],
    deps = [
        ":domain_naming_conf",
        ":domain_naming_conf_parser",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "loadbalance_naming_conf",
    srcs = ["loadbalance_naming_conf.cc"],
    hdrs = ["loadbalance_naming_conf.h"],
    deps = [
        "//trpc/util/log:logging",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

cc_library(
    name = "loadbalance_naming_conf_parser",
    hdrs = ["loadbalance_naming_conf_parser.h"],
    deps = [
        ":loadbalance_naming_conf",
    ],
)

cc_library(
    name = "local_file_provider_conf",
    srcs = ["local_file_provider_conf.cc"],
    hdrs = [
        "local_file_provider_conf.h",
        "local_file_provider_conf_parser.h",
    ],
    deps = [
        ":config_helper",
        "//trpc/util/log:logging",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)
