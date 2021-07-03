set(THIRDPARTIES_HAS_FETCHCONTENT ON)
# if(${CMAKE_VERSION} VERSION_LESS 3.11)
#     set(THIRDPARTIES_HAS_FETCHCONTENT OFF)
# endif()

if(THIRDPARTIES_HAS_FETCHCONTENT)
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
else()
    include(DownloadProject)
endif()

if(NOT FETCHCONTENT_BASE_DIR STREQUAL "")
    set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/3rdparty")
endif()

set(3rdparty_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_downloads" CACHE FILEPATH "3rdparty download directory.")
message(STATUS "base dir: ${FETCHCONTENT_BASE_DIR}")
message(STATUS "dnld dir: ${3rdparty_DOWNLOAD_DIR}")

macro(ThirdPartiesAdd_fmtlib)
    set(3rdparty_fmtlib_VERSION "8.0.1" CACHE STRING "fmtlib version")
    set(3rdparty_fmtlib_CHECKSUM "SHA256=b06ca3130158c625848f3fb7418f235155a4d389b2abc3a6245fb01cb0eb1e01" CACHE STRING "fmtlib checksum")
    set(3rdparty_fmtlib_URL "https://github.com/fmtlib/fmt/archive/refs/tags/${3rdparty_fmtlib_VERSION}.tar.gz")
    if(THIRDPARTIES_HAS_FETCHCONTENT)
        FetchContent_Declare(
            fmtlib
            URL "${3rdparty_fmtlib_URL}"
            URL_HASH "${3rdparty_fmtlib_CHECKSUM}"
            DOWNLOAD_DIR "${3rdparty_DOWNLOAD_DIR}"
            DOWNLOAD_NAME "fmtlib-${3rdparty_fmtlib_VERSION}.tar.gz"
            EXCLUDE_FROM_ALL
        )
        FetchContent_MakeAvailable(fmtlib)
    else()
        download_project(
            PROJ fmtlib
            URL "${3rdparty_fmtlib_URL}"
            URL_HASH "${3rdparty_fmtlib_CHECKSUM}"
            PREFIX "${FETCHCONTENT_BASE_DIR}/fmtlib-${3rdparty_fmtlib_VERSION}"
            DOWNLOAD_DIR "${3rdparty_DOWNLOAD_DIR}"
            DOWNLOAD_NAME "fmtlib-${3rdparty_fmtlib_VERSION}.tar.gz"
            EXCLUDE_FROM_ALL
            UPDATE_DISCONNECTED 1
        )
    endif()
endmacro()

macro(ThirdPartiesAdd_Catch2)
    set(3rdparty_Catch2_VERSION "bf61a418cbc4d3b430e3d258c5287780944ad505" CACHE STRING "Embedded catch2 version")
    set(3rdparty_Catch2_CHECKSUM "SHA256=7521e7e7ee7f2d301a639bdfe4a95855fbe503417d73af0934f9d1933ca38407" CACHE STRING "Embedded catch2 checksum")
    set(3rdparty_Catch2_NAME "catch2-${3rdparty_Catch2_VERSION}.zip" CACHE STRING "Embedded Catch2 download name")
    set(3rdparty_Catch2_URL "https://github.com/catchorg/Catch2/archive/${3rdparty_Catch2_VERSION}.zip" CACHE STRING "Embedded Catch2 URL")

    set(CATCH_BUILD_EXAMPLES OFF CACHE INTERNAL "")
    set(CATCH_BUILD_EXTRA_TESTS OFF CACHE INTERNAL "")
    set(CATCH_BUILD_TESTING OFF CACHE INTERNAL "")
    set(CATCH_ENABLE_WERROR OFF CACHE INTERNAL "")
    set(CATCH_INSTALL_DOCS OFF CACHE INTERNAL "")
    set(CATCH_INSTALL_HELPERS OFF CACHE INTERNAL "")
    if(THIRDPARTIES_HAS_FETCHCONTENT)
        FetchContent_Declare(
            Catch2
            URL "${3rdparty_Catch2_URL}"
            URL_HASH "${3rdparty_Catch2_CHECKSUM}"
            DOWNLOAD_DIR "${3rdparty_DOWNLOAD_DIR}"
            DOWNLOAD_NAME "${3rdparty_Catch2_NAME}"
            EXCLUDE_FROM_ALL
        )
        FetchContent_MakeAvailable(Catch2)
    else()
        download_project(
            PROJ Catch2
            URL "${3rdparty_Catch2_URL}"
            URL_HASH "${3rdparty_Catch2_CHECKSUM}"
            PREFIX "${FETCHCONTENT_BASE_DIR}/Catch2-${3rdparty_Catch2_VERSION}"
            DOWNLOAD_DIR "${3rdparty_DOWNLOAD_DIR}"
            DOWNLOAD_NAME "${3rdparty_Catch2_NAME}"
            EXCLUDE_FROM_ALL
        )
    endif()
endmacro()

macro(ThirdPartiesAdd_range_v3)
    set(3rdparty_range_v3_VERSION "0.11.0" CACHE STRING "Embedded range-v3 version")
    set(3rdparty_range_v3_CHECKSUM "MD5=97ab1653f3aa5f9e3d8200ee2a4911d3" CACHE STRING "Embedded range-v3 hash")
    if(THIRDPARTIES_HAS_FETCHCONTENT)
        FetchContent_GetProperties(range_v3)
        if(NOT "${range_v3_POPULATED}")
            FetchContent_Declare(
                range_v3
                URL "https://github.com/ericniebler/range-v3/archive/${3rdparty_range_v3_VERSION}.tar.gz"
                URL_HASH "${3rdparty_range_v3_CHECKSUM}"
                DOWNLOAD_DIR "${3rdparty_DOWNLOAD_DIR}"
                DOWNLOAD_NAME "range-v3-${3rdparty_range_v3_VERSION}.tar.gz"
                EXCLUDE_FROM_ALL
            )
            FetchContent_Populate(range_v3)
            add_subdirectory(${range_v3_SOURCE_DIR} ${range_v3_BINARY_DIR} EXCLUDE_FROM_ALL)
            # ^^^ That's the only way to avoid installing this dependency during install step.
        endif()
    else()
        download_project(
            PROJ range-v3
            URL "https://github.com/ericniebler/range-v3/archive/${3rdparty_range_v3_VERSION}.tar.gz"
            URL_HASH "${3rdparty_range_v3_CHECKSUM}"
            DOWNLOAD_DIR "${3rdparty_DOWNLOAD_DIR}"
            DOWNLOAD_NAME "range-v3-${3rdparty_range_v3_VERSION}.tar.gz"
            EXCLUDE_FROM_ALL
            PREFIX "${FETCHCONTENT_BASE_DIR}/range-v3-${3rdparty_range_v3_VERSION}"
        )
    endif()
endmacro()

# {{{ yaml-cpp
macro(ThirdPartiesAdd_yaml_cpp)
    set(3rdparty_yaml_cpp_VERSION "a6bbe0e50ac4074f0b9b44188c28cf00caf1a723" CACHE STRING "Embedded yaml-cpp version")
    set(3rdparty_yaml_cpp_CHECKSUM "SHA256=03d214d71b8bac32f684756003eb47a335fef8f8152d0894cf06e541eaf1c7f4" CACHE STRING "Embedded yaml-cpp checksum")
    set(3rdparty_yaml_cpp_NAME "yaml-cpp-${3rdparty_yaml_cpp_VERSION}.zip" CACHE STRING "Embedded yaml-cpp download name")
    set(3rdparty_yaml_cpp_URL "https://github.com/jbeder/yaml-cpp/archive/${3rdparty_yaml_cpp_VERSION}.zip" CACHE STRING "Embedded yaml-cpp URL")
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE INTERNAL "")
    set(YAML_CPP_BUILD_TESTS OFF CACHE INTERNAL "")
    set(YAML_CPP_BUILD_TOOLS OFF CACHE INTERNAL "")
    set(YAML_CPP_INSTALL OFF CACHE INTERNAL "")
    if(THIRDPARTIES_HAS_FETCHCONTENT)
        FetchContent_Declare(
            yaml-cpp
            URL "${3rdparty_yaml_cpp_URL}"
            URL_HASH "${3rdparty_yaml_cpp_CHECKSUM}"
            DOWNLOAD_DIR "${3rdparty_DOWNLOAD_DIR}"
            DOWNLOAD_NAME "${3rdparty_yaml_cpp_NAME}"
            EXCLUDE_FROM_ALL
        )
        FetchContent_MakeAvailable(yaml-cpp)
    else()
        download_project(
            PROJ yaml-cpp
            URL "${3rdparty_yaml_cpp_URL}"
            URL_HASH "${3rdparty_yaml_cpp_CHECKSUM}"
            DOWNLOAD_DIR "${3rdparty_DOWNLOAD_DIR}"
            DOWNLOAD_NAME "${3rdparty_yaml_cpp_NAME}"
            EXCLUDE_FROM_ALL
            PREFIX "${FETCHCONTENT_BASE_DIR}/yaml-cpp-${3rdparty_yaml_cpp_VERSION}"
        )
    endif()
endmacro()
# }}}

# {{{ libunicode
macro(ThirdPartiesAdd_libunicode)
    set(3rdparty_libunicode_VERSION "efd6d1cc29f73109af846f29d30c6977f5beb4e3" CACHE STRING "libunicode: commit hash")
    set(3rdparty_libunicode_CHECKSUM "SHA256=8151bbe809f0e4c26fc0f6c9a350550865e93b9bab69d4e8f287fc6113f27c59" CACHE STRING "libunicode: download checksum")
    set(3rdparty_libunicode_NAME "libunicode-${3rdparty_libunicode_VERSION}.zip" CACHE STRING "Embedded libunicode download name")
    set(3rdparty_libunicode_URL "https://github.com/contour-terminal/libunicode/archive/${3rdparty_libunicode_VERSION}.zip" CACHE STRING "Embedded libunicode URL")
    if(THIRDPARTIES_HAS_FETCHCONTENT)
        FetchContent_Declare(
            libunicode
            URL "${3rdparty_libunicode_URL}"
            URL_HASH "${3rdparty_libunicode_CHECKSUM}"
            DOWNLOAD_DIR "${3rdparty_DOWNLOAD_DIR}"
            DOWNLOAD_NAME "${3rdparty_libunicode_NAME}"
            UPDATE_DISCONNECTED 0
            EXCLUDE_FROM_ALL
        )
        FetchContent_MakeAvailable(libunicode)
    else()
        download_project(
            PROJ libunicode
            URL "${3rdparty_libunicode_URL}"
            URL_HASH "${3rdparty_libunicode_CHECKSUM}"
            DOWNLOAD_DIR "${3rdparty_DOWNLOAD_DIR}"
            DOWNLOAD_NAME "${3rdparty_libunicode_NAME}"
            EXCLUDE_FROM_ALL
            PREFIX "${FETCHCONTENT_BASE_DIR}/libunicode-${3rdparty_libunicode_VERSION}"
        )
    endif()
endmacro()
# }}}

