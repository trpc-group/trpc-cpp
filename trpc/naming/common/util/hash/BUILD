# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "hash_func",
    srcs =["hash_func.cc"],
    hdrs = ["hash_func.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming/common/util/hash:MurmurHash3",
        "//trpc/naming/common/util/hash:City",
        "//trpc/naming/common/util/hash:Md5",
    ],
)

cc_library(
    name="MurmurHash3",
    srcs=["MurmurHash3.cc"],
    hdrs=["MurmurHash3.h"],
    visibility=[
        "//visibility:public",
    ],
    deps=[],
)

cc_library(
    name="City",
    srcs=["City.cc"],
    hdrs=["City.h"],
    visibility=[
        "//visibility:public",
    ],
    deps=[],
)

cc_library(
    name="Md5",
    srcs=["Md5.cc"],
    hdrs=["Md5.h"],
    visibility=[
        "//visibility:public",
    ],
    deps=[],
)