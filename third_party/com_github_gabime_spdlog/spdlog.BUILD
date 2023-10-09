package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "spdlog",
    srcs = glob(["src/*.cpp"]),
    hdrs = glob([
        "include/spdlog/*.h",
        "include/spdlog/*/*.h",
        "include/spdlog/*/*/*.h",
    ]),
    defines = [
        "SPDLOG_COMPILED_LIB",
        "SPDLOG_FMT_EXTERNAL",
        "SPDLOG_DISABLE_DEFAULT_LOGGER",
    ],
    strip_include_prefix = "include",
    deps = ["@com_github_fmtlib_fmt//:fmtlib"],
)
