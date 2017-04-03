cmake_minimum_required(VERSION 2.8)
project(xtracer)

set(PATH_ROOT ${CMAKE_SOURCE_DIR})
set(PATH_SRC  ${PATH_ROOT}/src)
set(PATH_BIN  ${PATH_ROOT}/bin)
set(PATH_LIB  ${PATH_ROOT}/lib)
set(PATH_EXT  ${PATH_ROOT}/ext)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_CACHEFILE_DIR ${PATH_BIN})

set(PATH_SRC_XTCORE ${PATH_SRC}/xtcore)
set(PATH_SRC_CLI ${PATH_SRC}/xt_cli)
set(PATH_SRC_GUI ${PATH_SRC}/xt_gui)

file(GLOB SRC_LIBS
    ${PATH_LIB}/*/*.c
    ${PATH_LIB}/*/*.cc
    ${PATH_LIB}/*/*.cpp
    ${PATH_LIB}/*/*.c11
)

file(GLOB SRC_XTCORE
    ${PATH_SRC_XTCORE}/*.c
    ${PATH_SRC_XTCORE}/*.cc
    ${PATH_SRC_XTCORE}/*.cpp
    ${PATH_SRC_XTCORE}/*.c11
    ${PATH_SRC_XTCORE}/*/*.c
    ${PATH_SRC_XTCORE}/*/*.cc
    ${PATH_SRC_XTCORE}/*/*.cc
    ${PATH_SRC_XTCORE}/*/*.cpp
    ${PATH_SRC_XTCORE}/*/*.c11
)

include_directories(
    ${PATH_LIB}
    ${PATH_EXT}
)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

add_library(xtcore_static STATIC ${SRC_XTCORE})
add_library(xtcore_shared SHARED ${SRC_XTCORE})
