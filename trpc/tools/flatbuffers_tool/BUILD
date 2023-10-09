licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "schema_interface",
    srcs = [],
    hdrs = ["schema_interface.h"],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "parse_fbs_file",
    srcs = ["parse_fbs_file.cc"],
    hdrs = ["parse_fbs_file.h"],
    visibility = ["//visibility:private"],
    deps = [
        ":schema_interface",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_library(
    name = "fbs2cpp",
    srcs = ["fbs2cpp.cc"],
    hdrs = ["fbs2cpp.h"],
    visibility = ["//visibility:private"],
    deps = [
        ":schema_interface",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_test(
    name = "fbs2cpp_test",
    srcs = ["fbs2cpp_test.cc"],
    deps = [
        ":fbs2cpp",
        ":schema_interface",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "gen_trpc",
    srcs = ["idl_gen_trpc.cc"],
    hdrs = ["idl_gen_trpc.h"],
    visibility = ["//visibility:private"],
    deps = [
        ":fbs2cpp",
        ":parse_fbs_file",
        "@com_github_google_flatbuffers//:flatbuffers",
    ],
)

cc_binary(
    name = "fbs_cpp_plugin",
    srcs = [
        "main.cc",
    ],
    copts = [
        "-std=c++17",
    ],
    deps = [
        ":gen_trpc",
        "@com_github_google_flatbuffers//:flatbuffers",
        "@com_github_google_flatbuffers//:flatc_library",
        "@com_github_google_flatbuffers//:flatc_main",
    ],
)

cc_library(
    name = "fbs_gen_test_fixture",
    hdrs = ["fbs_gen_test_fixture.h"],
    deps = [
        "@com_github_google_flatbuffers//:flatbuffers",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "idl_gen_trpc_test",
    srcs = ["idl_gen_trpc_test.cc"],
    data = [
        "//trpc/tools/flatbuffers_tool/testing:greeter.fbs",
        "//trpc/tools/flatbuffers_tool/testing:greeter.trpc.fb_cc.txt",
        "//trpc/tools/flatbuffers_tool/testing:greeter.trpc.fb_h.txt",
    ],
    deps = [
        ":fbs_gen_test_fixture",
        ":gen_trpc",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "parse_fbs_file_test",
    srcs = ["parse_fbs_file_test.cc"],
    data = [
        "//trpc/tools/flatbuffers_tool/testing:greeter.fbs",
        "//trpc/tools/flatbuffers_tool/testing:greeter.trpc.fb_cc.txt",
        "//trpc/tools/flatbuffers_tool/testing:greeter.trpc.fb_h.txt",
    ],
    deps = [
        ":fbs_gen_test_fixture",
        ":parse_fbs_file",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
