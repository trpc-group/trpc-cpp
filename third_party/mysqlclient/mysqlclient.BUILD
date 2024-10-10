# mysql.BUILD

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "mysqlclient_deps",
    # Exclude libprotobuf to avoid conflicts with the existing protobuf in the project.
    srcs = glob(["lib/private/*"],
            exclude = ["lib/private/libproto*"]),
    visibility = ["//visibility:private"],
)

cc_library(
    name = "mysqlclient",
    srcs = glob(["lib/libmysqlclient.*"]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    deps = [":mysqlclient_deps"],
    strip_include_prefix = "include",
    include_prefix = "mysqlclient",
    visibility = ["//visibility:public"],
)

