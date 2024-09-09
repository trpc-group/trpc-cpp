#
#
# Tencent is pleased to support the open source community by making tRPC available.
#
# Copyright (C) 2023 THL A29 Limited, a Tencent company.
# All rights reserved.
#
# If you have downloaded a copy of the tRPC source code from Tencent,
# please note that tRPC source code is licensed under the  Apache 2.0 License,
# A copy of the Apache 2.0 License is included in this file.
#
#
"""This module contains some dependency"""

# buildifier: disable=load
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def clean_dep(dep):
    return str(Label(dep))

# buildifier: disable=function-docstring-args
def trpc_workspace(path_prefix = "", repo_name = "", **kwargs):
    """Build rules for the trpc project

    Note: The main idea is to determine the required version of dependent software during the build process
          by passing in parameters.
    Args:
        path_prefix: Path prefix.
        repo_name: Repository name of the dependency.
        kwargs: Keyword arguments, dictionary type, mainly used to specify the version and sha256 value of
                dependent software, where the key of the keyword is constructed by the `name + version`.
                eg: protobuf_ver,zlib_ver...
    Example:
        trpc_workspace(path_prefix="", repo_name="", protobuf_ver="xxx", protobuf_sha256="xxx", ...)
        Here, `xxx` is the specific specified version. If the version is not specified through the key,
        the default value will be used. eg: protobuf_ver = kwargs.get("protobuf_ver", "3.8.0")
    """

    # bazel_skylib version and summary.
    bazel_skylib_ver = kwargs.get("bazel_skylib_ver", "1.0.2")
    bazel_skylib_sha256 = kwargs.get("bazel_skylib_sha256", "e5d90f0ec952883d56747b7604e2a15ee36e288bb556c3d0ed33e818a4d971f2")
    bazel_skylib_urls = [
        "https://github.com/bazelbuild/bazel-skylib/archive/{ver}.tar.gz".format(ver = bazel_skylib_ver),
    ]
    if not native.existing_rule("bazel_skylib"):
        http_archive(
            name = "bazel_skylib",
            sha256 = bazel_skylib_sha256,
            strip_prefix = "bazel-skylib-{ver}".format(ver = bazel_skylib_ver),
            urls = bazel_skylib_urls,
        )

    # rules_python version and summary（required for higher versions of protobuf.）
    rules_python_ver = kwargs.get("rules_python_ver", "4b84ad270387a7c439ebdccfd530e2339601ef27")
    rules_python_sha256 = kwargs.get("rules_python_sha256", "e5470e92a18aa51830db99a4d9c492cc613761d5bdb7131c04bd92b9834380f6")
    rules_python_urls = [
        "https://github.com/bazelbuild/rules_python/archive/{ver}.tar.gz".format(ver = rules_python_ver),
    ]
    if not native.existing_rule("rules_python"):
        http_archive(
            name = "rules_python",
            sha256 = rules_python_sha256,
            strip_prefix = "rules_python-{ver}".format(ver = rules_python_ver),
            urls = rules_python_urls,
        )

    # zlib
    zlib_ver = kwargs.get("zlib_ver", "1.2.11")
    zlib_sha256 = kwargs.get("zlib_sha256", "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff")
    zlib_urls = [
        "https://github.com/madler/zlib/archive/v{ver}.tar.gz".format(ver = zlib_ver),
    ]
    if not native.existing_rule("zlib"):
        http_archive(
            name = "zlib",
            build_file = clean_dep("//third_party/zlib:zlib.BUILD"),
            sha256 = zlib_sha256,
            strip_prefix = "zlib-{ver}".format(ver = zlib_ver),
            urls = zlib_urls,
        )

    # com_github_google_snappy
    com_github_google_snappy_ver = kwargs.get("com_github_google_snappy_ver", "1.1.8")
    com_github_google_snappy_sha256 = kwargs.get("com_github_google_snappy_sha256", "16b677f07832a612b0836178db7f374e414f94657c138e6993cbfc5dcc58651f")
    com_github_google_snappy_name = "snappy-{ver}".format(ver = com_github_google_snappy_ver)
    com_github_google_snappy_urls = [
        "https://github.com/google/snappy/archive/refs/tags/{ver}.tar.gz".format(ver = com_github_google_snappy_ver),
    ]
    http_archive(
        name = "com_github_google_snappy",
        build_file = clean_dep("//third_party/com_github_google_snappy:snappy.BUILD"),
        sha256 = com_github_google_snappy_sha256,
        strip_prefix = com_github_google_snappy_name,
        urls = com_github_google_snappy_urls,
    )

    # com_github_lz4_lz
    com_github_lz4_lz4_ver = kwargs.get("com_github_lz4_lz4_ver", "1.9.4")
    com_github_lz4_lz4_sha256 = kwargs.get("com_github_lz4_lz4_sha256", "0b0e3aa07c8c063ddf40b082bdf7e37a1562bda40a0ff5272957f3e987e0e54b")
    com_github_lz4_lz4_name = "lz4-{ver}".format(ver = com_github_lz4_lz4_ver)
    com_github_lz4_lz4_urls = [
        "https://github.com/lz4/lz4/archive/refs/tags/v{ver}.tar.gz".format(ver = com_github_lz4_lz4_ver),
    ]
    http_archive(
        name = "com_github_lz4_lz4",
        build_file = clean_dep("//third_party/com_github_lz4_lz4:lz4.BUILD"),
        sha256 = com_github_lz4_lz4_sha256,
        strip_prefix = com_github_lz4_lz4_name,
        urls = com_github_lz4_lz4_urls,
    )

    # protobuf version and summary
    com_google_protobuf_ver = kwargs.get("com_google_protobuf_ver", "3.15.8")
    com_google_protobuf_sha256 = kwargs.get("com_google_protobuf_sha256", "0cbdc9adda01f6d2facc65a22a2be5cecefbefe5a09e5382ee8879b522c04441")
    com_google_protobuf_strip_prefix = "protobuf-{ver}".format(ver = com_google_protobuf_ver)
    com_google_protobuf_urls = [
        "https://github.com/protocolbuffers/protobuf/archive/v{ver}.tar.gz".format(ver = com_google_protobuf_ver),
    ]
    http_archive(
        name = "com_google_protobuf",
        sha256 = com_google_protobuf_sha256,
        strip_prefix = com_google_protobuf_strip_prefix,
        urls = com_google_protobuf_urls,
    )

    # com_google_googletest
    com_google_googletest_ver = kwargs.get("com_google_googletest_ver", "1.11.0")
    com_google_googletest_sha256 = kwargs.get("com_google_googletest_sha256", "b4870bf121ff7795ba20d20bcdd8627b8e088f2d1dab299a031c1034eddc93d5")
    com_google_googletest_urls = [
        "https://github.com/google/googletest/archive/release-{ver}.tar.gz".format(ver = com_google_googletest_ver),
    ]
    http_archive(
        name = "com_google_googletest",
        sha256 = com_google_googletest_sha256,
        strip_prefix = "googletest-release-{ver}".format(ver = com_google_googletest_ver),
        urls = com_google_googletest_urls,
    )

    # com_github_gflags_gflags
    com_github_gflags_gflags_ver = kwargs.get("com_github_gflags_gflags_ver", "2.2.2")
    com_github_gflags_gflags_sha256 = kwargs.get("com_github_gflags_gflags_sha256", "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf")
    com_github_gflags_gflags_urls = [
        "https://github.com/gflags/gflags/archive/v{ver}.tar.gz".format(ver = com_github_gflags_gflags_ver),
    ]
    http_archive(
        name = "com_github_gflags_gflags",
        sha256 = com_github_gflags_gflags_sha256,
        strip_prefix = "gflags-{ver}".format(ver = com_github_gflags_gflags_ver),
        urls = com_github_gflags_gflags_urls,
    )

    # com_github_jbeder_yaml_cpp
    com_github_jbeder_yaml_cpp_ver = kwargs.get("com_github_jbeder_yaml_cpp_ver", "0.7.0")
    com_github_jbeder_yaml_cpp_sha256 = kwargs.get("com_github_jbeder_yaml_cpp_sha256", "43e6a9fcb146ad871515f0d0873947e5d497a1c9c60c58cb102a97b47208b7c3")
    com_github_jbeder_yaml_cpp_urls = [
        "https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-{ver}.tar.gz".format(ver = com_github_jbeder_yaml_cpp_ver),
    ]
    http_archive(
        name = "com_github_jbeder_yaml_cpp",
        sha256 = com_github_jbeder_yaml_cpp_sha256,
        strip_prefix = "yaml-cpp-yaml-cpp-{ver}".format(ver = com_github_jbeder_yaml_cpp_ver),
        urls = com_github_jbeder_yaml_cpp_urls,
    )

    # com_github_gabime_spdlog
    com_github_gabime_spdlog_ver = kwargs.get("com_github_gabime_spdlog_ver", "1.10.0")
    com_github_gabime_spdlog_sha256 = kwargs.get("com_github_gabime_spdlog_sha256", "697f91700237dbae2326b90469be32b876b2b44888302afbc7aceb68bcfe8224")
    com_github_gabime_spdlog_urls = [
        "https://github.com/gabime/spdlog/archive/refs/tags/v{ver}.tar.gz".format(ver = com_github_gabime_spdlog_ver),
    ]
    http_archive(
        name = "com_github_gabime_spdlog",
        sha256 = com_github_gabime_spdlog_sha256,
        strip_prefix = "spdlog-{ver}".format(ver = com_github_gabime_spdlog_ver),
        urls = com_github_gabime_spdlog_urls,
        build_file = clean_dep("//third_party/com_github_gabime_spdlog:spdlog.BUILD"),
        patches = [
            "@trpc_cpp//third_party/com_github_gabime_spdlog:0001-flush-unit-change-for-milliseconds.patch",
            "@trpc_cpp//third_party/com_github_gabime_spdlog:0002-pre-allocate-file-buffer-size.patch",
        ],
        patch_args = ["-p1"],
    )

    # com_github_fmtlib_fmt
    com_github_fmtlib_fmt_ver = kwargs.get("com_github_fmtlib_fmt_ver", "9.1.0")
    com_github_fmtlib_fmt_sha256 = kwargs.get("com_github_fmtlib_fmt_sha256", "5dea48d1fcddc3ec571ce2058e13910a0d4a6bab4cc09a809d8b1dd1c88ae6f2")
    com_github_fmtlib_fmt_urls = [
        "https://github.com/fmtlib/fmt/archive/{ver}.tar.gz".format(ver = com_github_fmtlib_fmt_ver),
    ]
    http_archive(
        name = "com_github_fmtlib_fmt",
        sha256 = com_github_fmtlib_fmt_sha256,
        strip_prefix = "fmt-{ver}".format(ver = com_github_fmtlib_fmt_ver),
        urls = com_github_fmtlib_fmt_urls,
        build_file = clean_dep("//third_party/com_github_fmtlib_fmt:fmtlib.BUILD"),
    )

    #zlib version and summary
    zlib_ver = kwargs.get("zlib_ver", "1.2.11")
    zlib_sha256 = kwargs.get("zlib_sha256", "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff")
    zlib_urls = [
        "https://github.com/madler/zlib/archive/v{ver}.tar.gz".format(ver = zlib_ver),
    ]
    http_archive(
        name = "com_github_madler_zlib",
        build_file = clean_dep("//third_party/com_github_madler_zlib:zlib.BUILD"),
        sha256 = zlib_sha256,
        strip_prefix = "zlib-{ver}".format(ver = zlib_ver),
        urls = zlib_urls,
    )

    native.new_local_repository(
        name = "com_github_openssl_openssl",
        path = "/usr",
        build_file = clean_dep("//third_party/com_github_openssl_openssl:openssl.BUILD"),
    )

    # com_github_tencent_rapidjson
    com_github_tencent_rapidjson_ver = kwargs.get("com_github_tencent_rapidjson_ver", "1.1.0")
    com_github_tencent_rapidjson_sha256 = kwargs.get("com_github_tencent_rapidjson_sha256", "8e00c38829d6785a2dfb951bb87c6974fa07dfe488aa5b25deec4b8bc0f6a3ab")
    com_github_tencent_rapidjson_urls = [
        "https://github.com/Tencent/rapidjson/archive/v{ver}.zip".format(ver = com_github_tencent_rapidjson_ver),
    ]
    http_archive(
        name = "com_github_tencent_rapidjson",
        sha256 = com_github_tencent_rapidjson_sha256,
        strip_prefix = "rapidjson-{ver}".format(ver = com_github_tencent_rapidjson_ver),
        urls = com_github_tencent_rapidjson_urls,
        build_file = clean_dep("//third_party/com_github_tencent_rapidjson:rapidjson.BUILD"),
    )

    # com_github_open_source_parsers_jsoncpp
    com_github_open_source_parsers_jsoncpp_ver = kwargs.get("com_github_open_source_parsers_jsoncpp_ver", "1.9.5")
    com_github_open_source_parsers_jsoncpp_sha256 = kwargs.get("com_github_open_source_parsers_jsoncpp_sha256", "f409856e5920c18d0c2fb85276e24ee607d2a09b5e7d5f0a371368903c275da2")
    com_github_open_source_parsers_jsoncpp_name = "jsoncpp-{ver}".format(ver = com_github_open_source_parsers_jsoncpp_ver)
    com_github_open_source_parsers_jsoncpp_urls = [
        "https://github.com/open-source-parsers/jsoncpp/archive/{ver}.tar.gz".format(ver = com_github_open_source_parsers_jsoncpp_ver),
    ]
    http_archive(
        name = "com_github_open_source_parsers_jsoncpp",
        strip_prefix = com_github_open_source_parsers_jsoncpp_name,
        sha256 = com_github_open_source_parsers_jsoncpp_sha256,
        urls = com_github_open_source_parsers_jsoncpp_urls,
    )

    new_git_repository(
        name = "com_github_h2o_picohttpparser",
        remote = "https://github.com/h2o/picohttpparser.git",
        branch = "master",
        build_file = clean_dep("//third_party/com_github_h2o_picohttpparser:picohttpparser.BUILD"),
    )

    civetweb_ver = kwargs.get("civetweb_ver", "1.15")
    civetweb_sha256 = kwargs.get("civetweb_sha256", "90a533422944ab327a4fbb9969f0845d0dba05354f9cacce3a5005fa59f593b9")
    civetweb_urls = [
        "https://mirrors.tencent.com/github.com/civetweb/civetweb/archive/v{ver}.tar.gz".format(ver = civetweb_ver),
        "https://github.com/civetweb/civetweb/archive/v{ver}.tar.gz".format(ver = civetweb_ver),
    ]
    http_archive(
        name = "civetweb",
        build_file = clean_dep("@com_github_jupp0r_prometheus_cpp//bazel:civetweb.BUILD"),
        sha256 = civetweb_sha256,
        strip_prefix = "civetweb-{ver}".format(ver = civetweb_ver),
        urls = civetweb_urls,
    )

    http_archive(
        name = "net_zlib_zlib",
        build_file = clean_dep("@com_github_jupp0r_prometheus_cpp//bazel:zlib.BUILD"),
        sha256 = zlib_sha256,
        strip_prefix = "zlib-{ver}".format(ver = zlib_ver),
        urls = zlib_urls,
    )

    # com_github_jupp0r_prometheus_cpp
    com_github_jupp0r_prometheus_cpp_ver = kwargs.get("com_github_jupp0r_prometheus_cpp_ver", "1.1.0")
    com_github_jupp0r_prometheus_cpp_sha256 = kwargs.get("com_github_jupp0r_prometheus_cpp_sha256", "397544fe91e183029120b4eebcfab24ed9ec833d15850aae78fd5db19062d13a")
    com_github_jupp0r_prometheus_cpp_urls = [
        "https://github.com/jupp0r/prometheus-cpp/archive/v{ver}.tar.gz".format(ver = com_github_jupp0r_prometheus_cpp_ver),
    ]
    http_archive(
        name = "com_github_jupp0r_prometheus_cpp",
        sha256 = com_github_jupp0r_prometheus_cpp_sha256,
        strip_prefix = "prometheus-cpp-{ver}".format(ver = com_github_jupp0r_prometheus_cpp_ver),
        urls = com_github_jupp0r_prometheus_cpp_urls,
    )

    # com_github_nghttp2_nghttp2
    # url: https://github.com/nghttp2/nghttp2/releases/download/v1.40.0/nghttp2-1.40.0.tar.gz
    # sha256: eb9d9046495a49dd40c7ef5d6c9907b51e5a6b320ea6e2add11eb8b52c982c47
    nghttp2_ver = kwargs.get("nghttp2_ver", "1.40.0")
    nghttp2_sha256 = kwargs.get("nghttp2_sha256", "eb9d9046495a49dd40c7ef5d6c9907b51e5a6b320ea6e2add11eb8b52c982c47")
    nghttp2_name = "nghttp2-{ver}".format(ver = nghttp2_ver)
    nghttp2_urls = [
        "https://mirrors.tencent.com/github.com/nghttp2/nghttp2/releases/download/v{ver}/nghttp2-{ver}.tar.gz".format(ver = nghttp2_ver),
        "https://github.com/nghttp2/nghttp2/releases/download/v{ver}/nghttp2-{ver}.tar.gz".format(ver = nghttp2_ver),
    ]
    http_archive(
        name = "nghttp2",
        build_file = clean_dep("//third_party/com_github_nghttp2_nghttp2:nghttp2.BUILD"),
        sha256 = nghttp2_sha256,
        strip_prefix = nghttp2_name,
        urls = nghttp2_urls,
    )

    # liburing
    liburing_ver = kwargs.get("liburing_ver", "2.1")
    liburing_sha256 = kwargs.get("liburing_sha256", "9669db5d9d48dafba0f3d033e0dce15aeee561bbd6b06eef4f392b122f75c33d")
    liburing_name = "liburing-liburing-{ver}".format(ver = liburing_ver)
    liburing_urls = [
        "https://github.com/axboe/liburing/archive/refs/tags/liburing-{ver}.zip".format(ver = liburing_ver),
    ]
    http_archive(
        name = "liburing",
        build_file = clean_dep("//third_party/com_github_axboe_liburing:liburing.BUILD"),
        sha256 = liburing_sha256,
        strip_prefix = liburing_name,
        urls = liburing_urls,
    )

    http_archive(
        name = "com_github_toruniina_toml11",
        strip_prefix = "toml11-3.7.0",
        urls = ["https://github.com/ToruNiina/toml11/archive/refs/tags/v3.7.0.tar.gz"],
        sha256 = "a0b6bec77c0e418eea7d270a4437510884f2fe8f61e7ab121729624f04c4b58e",
        build_file = clean_dep("//third_party/com_github_toruniina_toml11:toml.BUILD"),
    )
    # com_github_google_flatbuffers
    fbs_ver = kwargs.get("fbs_ver", "23.5.26")
    fbs_sha256 = kwargs.get("fbs_sha256", "1cce06b17cddd896b6d73cc047e36a254fb8df4d7ea18a46acf16c4c0cd3f3f3")
    fbs_urls = [
       "https://github.com/google/flatbuffers/archive/v{ver}.tar.gz".format(ver = fbs_ver),
    ]
    http_archive(
        name = "com_github_google_flatbuffers",
        sha256 = fbs_sha256,
        strip_prefix = "flatbuffers-{ver}".format(ver = fbs_ver),
        urls = fbs_urls,
        build_file = clean_dep("//third_party/com_github_google_flatbuffers:flatbuffers.BUILD"),
        patches = [
            "@trpc_cpp//third_party/com_github_google_flatbuffers:0001-feat-Modify-bazel-file.patch",
        ],
        patch_args = ["-p1"],
    )

    # com_github_trpc_protocol
    git_repository(
        name = "com_github_trpc_protocol",
        remote = "https://github.com/trpc-group/trpc.git",
        tag = "v{ver}".format(ver = kwargs.get("com_github_trpc_protocol_ver", "1.0.0")),
    )

    # MySQL C API version and summary
    mysql_c_api_ver = kwargs.get("mysql_c_api_ver", "8.0.18")
    mysql_c_api_sha256 = kwargs.get("mysql_c_api_sha256", "c91970c20e9d868aa7bc90c4692b9bf1fd6ef12f604c3392b871ad4dfc04fb84")
    mysql_c_api_urls = [
        "https://dev.mysql.com/get/Downloads/Connector-C++/mysql-connector-c++-8.0.18-linux-glibc2.12-x86-64bit.tar.gz",
    ]


    http_archive(
        name = "mysql_c_api_archive",
        sha256 = mysql_c_api_sha256,
        strip_prefix = "mysql-connector-c++-8.0.18-linux-glibc2.12-x86-64bit",
        urls = mysql_c_api_urls,
        build_file = clean_dep("//third_party/mysql:mysql.BUILD"),
    )
