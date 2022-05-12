project(protop)

cmake_minimum_required(VERSION 3.10)

set(PROTOP_MAJOR_VERSION 3)
set(PROTOP_MINOR_VERSION 0)
set(PROTOP_PATCH_VERSION 0)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PROTOP_BUILD_DEBUG 1)
else()
    set(PROTOP_BUILD_RELEASE 1)
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()

if (UNIX)
    set(ENABLE_PROFILING OFF CACHE BOOL "")
    set(ENABLE_SANITIZER OFF CACHE BOOL "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wfatal-errors -fvisibility=hidden -pedantic -Wl,--no-undefined -fPIC -Wall -Wextra -Wconversion -Werror=return-type")

    if (ENABLE_PROFILING)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O3")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -O3")
    endif()

    set(CMAKE_CXX_FLAGS_DEBUG     "${CMAKE_C_FLAGS_DEBUG} -g -O0")

    if (ENABLE_SANITIZER)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fsanitize=address")
    endif()
endif()

add_library(libprotop STATIC
    "source/tokenizer.cc"
    "source/parser.cc"
    "source/exception.cc")
target_include_directories(libprotop PUBLIC "include")
set_target_properties(libprotop PROPERTIES
    OUTPUT_NAME "libprotop"
    VERSION "${PROTOP_MAJOR_VERSION}.${PROTOP_MINOR_VERSION}.${PROTOP_PATCH_VERSION}"
    SOVERSION "${PROTOP_MAJOR_VERSION}.${PROTOP_MINOR_VERSION}.${PROTOP_PATCH_VERSION}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    PREFIX "lib" )

add_executable(example_format
    "example/format/main.cc")
target_link_libraries(example_format libprotop)