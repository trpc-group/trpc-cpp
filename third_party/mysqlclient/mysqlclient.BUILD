# mysql.BUILD

package(
    default_visibility = ["//visibility:public"],
)

# 定义 MySQL C API 的库
# cc_library(
#     name = "mysql_c_api",
#     hdrs = glob(["include/**/*.h"]),
#     srcs = glob(["src/**/*.c"]),
#     includes = ["include"],
#     copts = ["-DDEFAULT_CHARSET=UTF8"],
#     visibility = ["//visibility:public"],  # 添加可见性设置
# )


cc_library(
    name = "mysqlclient",
    srcs = glob(["lib/libmysqlclient.*", "lib/private/*", "lib/private/sasl2/**"]),
    hdrs = glob([
        "include/**/*.h",
    ]),
    strip_include_prefix = "include",
    include_prefix = "mysqlclient",
    visibility = ["//visibility:public"],
)

