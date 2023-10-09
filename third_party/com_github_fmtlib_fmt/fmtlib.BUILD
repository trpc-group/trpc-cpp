cc_library(
    name = "fmtlib",
    srcs = [
        # NOTE: "src/fmt.cc" is not compiled and should not be included.
        "src/format.cc",
        "src/os.cc",
    ],
    hdrs = glob(["include/fmt/*.h"]),
    copts = [
        "-fvisibility=hidden",
        "-fvisibility-inlines-hidden",
        "-O3",
        "-std=gnu++11",
    ],
    local_defines = [
        "FMT_LOCALE",
        "FMT_EXPORT",
        "fmt_EXPORTS",
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)

exports_files([
    "test/unicode-test.cc",
    "test/util.h",
    "test/util.cc",
    "test/test-main.cc",
    "test/gtest-extra.cc",
    "test/gtest-extra.h",
])
