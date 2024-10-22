# mysqlclient.cmake

include(FetchContent)


if(NOT DEFINED MYSQLCLIENT_VERSION_TAG)
    set(MYSQLCLIENT_VERSION_TAG 8.0.39)
endif()

string(REGEX REPLACE "^([0-9]+\\.[0-9]+)\\..*" "\\1" MYSQLCLIENT_MAJOR_VER "${MYSQLCLIENT_VERSION_TAG}")
set(MYSQLCLIENT_URL "https://dev.mysql.com/get/Downloads/MySQL-${MYSQLCLIENT_MAJOR_VER}/mysql-${MYSQLCLIENT_VERSION_TAG}-linux-glibc2.17-x86_64-minimal.tar.xz")


FetchContent_Declare(
        mysqlclient
        URL         ${MYSQLCLIENT_URL}
        SOURCE_DIR  ${TRPC_ROOT_PATH}/cmake_third_party/mysqlclient
)

FetchContent_GetProperties(mysqlclient)
if(NOT mysqlclient_POPULATED)
    FetchContent_Populate(mysqlclient)
endif()

set(MYSQLCLIENT_INCLUDE_DIR "${mysqlclient_SOURCE_DIR}/include")
set(MYSQLCLIENT_LIB_DIR "${mysqlclient_SOURCE_DIR}/lib")


file(MAKE_DIRECTORY "${MYSQLCLIENT_INCLUDE_DIR}/mysqlclient")
file(GLOB MYSQL_HEADERS "${mysqlclient_SOURCE_DIR}/include/*.h")

# for #include "mysqlclient/mysql.h", otherwise we will directly #include "mysql.h"
file(COPY ${MYSQL_HEADERS} DESTINATION "${MYSQLCLIENT_INCLUDE_DIR}/mysqlclient")


add_library(mysqlclient STATIC IMPORTED)

set_target_properties(mysqlclient PROPERTIES
        IMPORTED_LOCATION "${MYSQLCLIENT_LIB_DIR}/libmysqlclient.a"
        INTERFACE_INCLUDE_DIRECTORIES "${MYSQLCLIENT_INCLUDE_DIR}"
)

target_link_libraries(mysqlclient INTERFACE
        "${MYSQLCLIENT_LIB_DIR}/private/libcrypto.so"
        "${MYSQLCLIENT_LIB_DIR}/private/libssl.so"
)

add_library(trpc_mysqlclient ALIAS mysqlclient)


set(TARGET_INCLUDE_PATHS ${TARGET_INCLUDE_PATHS} ${TRPC_ROOT_PATH}/cmake_third_party/mysqlclient/include)
set(TARGET_LINK_LIBS  ${TARGET_LINK_LIBS} trpc_mysqlclient)
