cmake_minimum_required(VERSION 3.1)
project(xtracer)

option(XTRACER_ENABLE_RTMIDI "Build with RtMidi/ALSA support" ON)
option(XTRACER_ENABLE_WEB "Build web frontend" ON)
option(XTRACER_ENABLE_WASM "Build standalone WASM frontend (Emscripten)" OFF)
option(XTRACER_ENABLE_WASM_DIST "Build standalone WASM dist into bin/wasm-dist during native builds" OFF)

set(XTRACER_USING_EMSCRIPTEN OFF)
if (EMSCRIPTEN OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    set(XTRACER_USING_EMSCRIPTEN ON)
endif()

# PATHS
set(PATH_ROOT ${CMAKE_SOURCE_DIR})
set(PATH_SRC  ${PATH_ROOT}/src)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PATH_BIN ${PATH_ROOT}/bin/debug)
else()
    set(PATH_BIN ${PATH_ROOT}/bin/release)
endif()
set(PATH_WEB  ${PATH_SRC}/frontend/web-client)
if (XTRACER_USING_EMSCRIPTEN)
    set(PATH_BIN ${PATH_ROOT}/bin-wasm)
endif()
set(PATH_LIB  ${PATH_ROOT}/lib)
set(PATH_EXT  ${PATH_ROOT}/ext)
set(PATH_TEST ${PATH_BIN}/test)

# Ensure output directories exist for all build modes.
file(MAKE_DIRECTORY ${PATH_BIN})
file(MAKE_DIRECTORY ${PATH_TEST})

if (XTRACER_ENABLE_WEB OR XTRACER_ENABLE_WASM OR XTRACER_ENABLE_WASM_DIST)
    set(PATH_WEB_LICENSE ${PATH_WEB}/license.txt)
    add_custom_command(
        OUTPUT ${PATH_WEB_LICENSE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PATH_WEB}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PATH_ROOT}/LICENSE ${PATH_WEB_LICENSE}
        DEPENDS ${PATH_ROOT}/LICENSE
        COMMENT "Generating ${PATH_WEB_LICENSE} from root LICENSE"
        VERBATIM
    )
    add_custom_target(web_license_asset DEPENDS ${PATH_WEB_LICENSE})
endif()

MACRO(SCAN_DIRS return_list)
    FILE(GLOB_RECURSE new_list *.h)
    SET(dir_list ${PATH_EXT})
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

execute_process(
    COMMAND git describe --long
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE DEF_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (XTRACER_USING_EMSCRIPTEN)
    set(OpenMP_C_FLAGS "")
    set(OpenMP_CXX_FLAGS "")
    set(OpenMP_EXE_LINKER_FLAGS "")
    set(OpenMP_LIBRARY "")
else()
    find_package(OpenMP REQUIRED)
endif()
SCAN_DIRS(ext_libs)

if (XTRACER_ENABLE_RTMIDI)
    find_package(ALSA)
    if (NOT ALSA_FOUND)
        message(WARNING "ALSA development files not found; disabling XTRACER_ENABLE_RTMIDI")
        set(XTRACER_ENABLE_RTMIDI OFF)
    endif()
endif()

set(PATH_SRC_XTCORE       ${PATH_SRC}/xtcore)
set(PATH_SRC_FRONTEND_CLI ${PATH_SRC}/frontend/cli)
set(PATH_SRC_FRONTEND_WEB ${PATH_SRC}/frontend/web-server)
set(PATH_SRC_FRONTEND_WASM ${PATH_SRC}/frontend/wasm)
set(PATH_SRC_FRONTEND_COMMON ${PATH_SRC}/frontend/common)
set(PATH_SRC_TESTS        ${PATH_SRC}/tests)

set(IN_MAN_CLI  xtracer_cli.1)

set(OUT_MAN_CLI xtracer_cli.1.gz)
set(OUT_LIB     xtcore)
set(OUT_EXE_CLI xtracer_cli)
set(OUT_EXE_COLORSPACE_TEST test_nimg_colorspace)
set(OUT_EXE_COLORSPACE_VECTORS_TEST test_nimg_colorspace_vectors)
set(OUT_EXE_TILE_TEST test_xtcore_tile)
set(OUT_EXE_CONTEXT_TEST test_xtcore_context)
set(OUT_EXE_SPHERE_TEST test_xtcore_sphere)
set(OUT_EXE_TRIANGLE_TEST test_xtcore_triangle)
set(OUT_EXE_WHITE_FURNACE_TEST test_xtcore_white_furnace)
set(OUT_EXE_RAYTRACER_EMISSIVE_TEST test_xtcore_raytracer_emissive)
set(OUT_EXE_CLI_SETUP_TEST test_xtracer_cli_setup)
set(OUT_EXE_NCF_PARSER_TEST test_ncf_parser)
set(OUT_EXE_NMATH_BENCH bench_nmath)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_FILES_DIRECTORY          ${PATH_BIN})
set(CMAKE_CACHEFILE_DIR            ${PATH_BIN})

if (XTRACER_ENABLE_RTMIDI)
    file(GLOB SRC_RTMIDI
        ${PATH_EXT}/rtmidi/*.cpp
    )
endif()

file(GLOB SRC_LIBS
    ${PATH_LIB}/*/*.c
    ${PATH_LIB}/*/*.cc
    ${PATH_LIB}/*/*.cpp
)

file(GLOB SRC_XTCORE
    ${PATH_SRC_XTCORE}/*.c
    ${PATH_SRC_XTCORE}/*.cc
    ${PATH_SRC_XTCORE}/*.cpp
    ${PATH_SRC_XTCORE}/*/*.c
    ${PATH_SRC_XTCORE}/*/*.cc
    ${PATH_SRC_XTCORE}/*/*.cpp
    ${PATH_SRC_XTCORE}/*/*/*.c
    ${PATH_SRC_XTCORE}/*/*/*.cc
    ${PATH_SRC_XTCORE}/*/*/*.cpp
)

file(GLOB SRC_FRONTEND_CLI
    ${PATH_SRC_FRONTEND_CLI}/*.c
    ${PATH_SRC_FRONTEND_CLI}/*.cc
    ${PATH_SRC_FRONTEND_CLI}/*.cpp
)

file(GLOB SRC_FRONTEND_WEB
    ${PATH_SRC_FRONTEND_WEB}/*.c
    ${PATH_SRC_FRONTEND_WEB}/*.cc
    ${PATH_SRC_FRONTEND_WEB}/*.cpp
)

file(GLOB SRC_FRONTEND_WASM
    ${PATH_SRC_FRONTEND_WASM}/*.c
    ${PATH_SRC_FRONTEND_WASM}/*.cc
    ${PATH_SRC_FRONTEND_WASM}/*.cpp
)

file(GLOB SRC_FRONTEND_COMMON
    ${PATH_SRC_FRONTEND_COMMON}/*.c
    ${PATH_SRC_FRONTEND_COMMON}/*.cc
    ${PATH_SRC_FRONTEND_COMMON}/*.cpp
)

add_custom_target(manpages_cli
    gzip -c ${CMAKE_SOURCE_DIR}/man/${IN_MAN_CLI} > ${PATH_BIN}/${OUT_MAN_CLI}
)

add_library(xtcore_static  STATIC ${SRC_XTCORE} ${SRC_LIBS})
set_target_properties(xtcore_static  PROPERTIES OUTPUT_NAME xtcore)

add_library(xtcore_dynamic SHARED ${SRC_XTCORE} ${SRC_LIBS})
set_target_properties(xtcore_dynamic PROPERTIES OUTPUT_NAME xtcore)

target_include_directories(xtcore_static PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(xtcore_static SYSTEM PUBLIC
    ${ext_libs}
)

target_include_directories(xtcore_dynamic PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(xtcore_dynamic SYSTEM PUBLIC
    ${ext_libs}
)

if (XTRACER_ENABLE_RTMIDI)
    add_library(rtmidi STATIC ${SRC_RTMIDI})
    set_target_properties(rtmidi PROPERTIES OUTPUT_NAME rtmidi)
    target_compile_definitions(rtmidi PRIVATE __LINUX_ALSA__)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(rtmidi PRIVATE -Wno-implicit-fallthrough)
    endif()
    if (ALSA_FOUND)
        target_include_directories(rtmidi PRIVATE ${ALSA_INCLUDE_DIRS})
    endif()
endif()

add_executable(${OUT_EXE_CLI} ${SRC_FRONTEND_CLI})
add_executable(${OUT_EXE_COLORSPACE_TEST} ${PATH_SRC_TESTS}/colorspace_test.cc)
add_executable(${OUT_EXE_COLORSPACE_VECTORS_TEST} ${PATH_SRC_TESTS}/colorspace_vectors_test.cc)
add_executable(${OUT_EXE_TILE_TEST} ${PATH_SRC_TESTS}/tile_test.cc)
add_executable(${OUT_EXE_CONTEXT_TEST} ${PATH_SRC_TESTS}/context_test.cc)
add_executable(${OUT_EXE_SPHERE_TEST} ${PATH_SRC_TESTS}/sphere_test.cc)
add_executable(${OUT_EXE_TRIANGLE_TEST} ${PATH_SRC_TESTS}/triangle_test.cc)
add_executable(${OUT_EXE_WHITE_FURNACE_TEST} ${PATH_SRC_TESTS}/white_furnace_test.cc)
add_executable(${OUT_EXE_RAYTRACER_EMISSIVE_TEST} ${PATH_SRC_TESTS}/raytracer_emissive_test.cc)
add_executable(${OUT_EXE_CLI_SETUP_TEST} ${PATH_SRC_TESTS}/cli_setup_test.cc ${PATH_SRC_FRONTEND_CLI}/argparse.cc)
add_executable(${OUT_EXE_NCF_PARSER_TEST} ${PATH_SRC_TESTS}/ncf_parser_test.cc)
add_executable(${OUT_EXE_NMATH_BENCH} ${PATH_SRC_TESTS}/nmath_bench.cc)
set_target_properties(${OUT_EXE_COLORSPACE_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_COLORSPACE_VECTORS_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_TILE_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_CONTEXT_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_SPHERE_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_TRIANGLE_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_WHITE_FURNACE_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_RAYTRACER_EMISSIVE_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_CLI_SETUP_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_NCF_PARSER_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_NMATH_BENCH} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN}
)

target_include_directories(${OUT_EXE_CLI} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_CLI} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_COLORSPACE_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_COLORSPACE_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_COLORSPACE_VECTORS_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_COLORSPACE_VECTORS_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_TILE_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_TILE_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_CONTEXT_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_CONTEXT_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_SPHERE_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_SPHERE_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_TRIANGLE_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_TRIANGLE_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_WHITE_FURNACE_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_WHITE_FURNACE_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_RAYTRACER_EMISSIVE_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_RAYTRACER_EMISSIVE_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_CLI_SETUP_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_CLI_SETUP_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_NCF_PARSER_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_NCF_PARSER_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_NMATH_BENCH} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_NMATH_BENCH} SYSTEM PUBLIC
    ${ext_libs}
)

if (XTRACER_ENABLE_RTMIDI)
    target_link_libraries(${OUT_EXE_CLI} PRIVATE xtcore_static rtmidi ${ALSA_LIBRARIES} pthread)
else()
    target_link_libraries(${OUT_EXE_CLI} PRIVATE xtcore_static pthread)
endif()
target_link_libraries(${OUT_EXE_COLORSPACE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_COLORSPACE_VECTORS_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_TILE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_CONTEXT_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_SPHERE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_TRIANGLE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_WHITE_FURNACE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_RAYTRACER_EMISSIVE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_CLI_SETUP_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_NCF_PARSER_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_NMATH_BENCH} PRIVATE xtcore_static pthread)

if (XTRACER_ENABLE_WEB)
    set(OUT_EXE_WEB xtracer_web)
    add_executable(${OUT_EXE_WEB} ${SRC_FRONTEND_WEB} ${SRC_FRONTEND_COMMON})
    target_include_directories(${OUT_EXE_WEB} PUBLIC
        ${PATH_LIB}
        ${PATH_SRC}
    )
    target_include_directories(${OUT_EXE_WEB} SYSTEM PUBLIC
        ${ext_libs}
    )
    if (XTRACER_ENABLE_RTMIDI)
        target_link_libraries(${OUT_EXE_WEB} PRIVATE xtcore_static rtmidi ${ALSA_LIBRARIES} pthread)
    else()
        target_link_libraries(${OUT_EXE_WEB} PRIVATE xtcore_static pthread)
    endif()
    if (TARGET web_license_asset)
        add_dependencies(${OUT_EXE_WEB} web_license_asset)
    endif()
endif()

if (XTRACER_ENABLE_WASM)
    if (NOT XTRACER_USING_EMSCRIPTEN)
        message(FATAL_ERROR "XTRACER_ENABLE_WASM requires Emscripten toolchain")
    endif()

    set(OUT_EXE_WASM xtracer_wasm)
    add_executable(${OUT_EXE_WASM} ${SRC_FRONTEND_WASM} ${SRC_FRONTEND_COMMON})
    target_include_directories(${OUT_EXE_WASM} PUBLIC
        ${PATH_LIB}
        ${PATH_SRC}
    )
    target_include_directories(${OUT_EXE_WASM} SYSTEM PUBLIC
        ${ext_libs}
    )
    target_link_libraries(${OUT_EXE_WASM} PRIVATE xtcore_static)
    set_target_properties(${OUT_EXE_WASM} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PATH_WEB}
    )
    target_link_options(${OUT_EXE_WASM} PRIVATE
        "SHELL:-sWASM=1"
        "SHELL:-sALLOW_MEMORY_GROWTH=1"
        "SHELL:-sFORCE_FILESYSTEM=1"
        "SHELL:-sENVIRONMENT=worker,web"
        "SHELL:-sEXPORTED_FUNCTIONS=['_xtracer_wasm_render_png','_xtracer_wasm_render_begin','_xtracer_wasm_render_step','_xtracer_wasm_render_tiles_done','_xtracer_wasm_render_tiles_total','_xtracer_wasm_render_is_done','_xtracer_wasm_render_snapshot_png','_xtracer_wasm_render_snapshot_exr','_xtracer_wasm_render_snapshot_hdr','_xtracer_wasm_free','_xtracer_wasm_get_last_error','_malloc','_free']"
        "SHELL:-sEXPORTED_RUNTIME_METHODS=['cwrap','ccall','FS','UTF8ToString']"
    )
    if (TARGET web_license_asset)
        add_dependencies(${OUT_EXE_WASM} web_license_asset)
    endif()
endif()

if (XTRACER_ENABLE_WASM_DIST)
    if (XTRACER_USING_EMSCRIPTEN)
        message(WARNING "XTRACER_ENABLE_WASM_DIST is ignored while building with Emscripten toolchain")
    else()
        find_program(EMCMAKE_PROGRAM emcmake)
        if (NOT EMCMAKE_PROGRAM)
            message(FATAL_ERROR "XTRACER_ENABLE_WASM_DIST requires emcmake in PATH")
        endif()

        set(WASM_DIST_BUILD_DIR ${PATH_ROOT}/build-wasm)
        set(WASM_DIST_OUTPUT_DIR ${PATH_BIN}/wasm-dist)

        add_custom_target(xtracer_wasm_dist ALL
            COMMAND ${CMAKE_COMMAND} -E rm -f ${WASM_DIST_BUILD_DIR}/CMakeCache.txt
            COMMAND ${CMAKE_COMMAND} -E rm -rf ${WASM_DIST_BUILD_DIR}/CMakeFiles
            COMMAND ${EMCMAKE_PROGRAM} cmake -S ${PATH_ROOT} -B ${WASM_DIST_BUILD_DIR}
                    -DXTRACER_ENABLE_WEB=OFF
                    -DXTRACER_ENABLE_RTMIDI=OFF
                    -DXTRACER_ENABLE_WASM=ON
                    -DXTRACER_ENABLE_WASM_DIST=OFF
            COMMAND ${CMAKE_COMMAND} --build ${WASM_DIST_BUILD_DIR} -j --target xtracer_wasm
            COMMAND ${PATH_ROOT}/util/package_wasm_standalone.sh ${WASM_DIST_OUTPUT_DIR}
            COMMENT "Building and packaging standalone WASM dist into ${WASM_DIST_OUTPUT_DIR}"
            VERBATIM
        )
    endif()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wno-unknown-pragmas")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(DEFS ${DEFS} XTCORE_VERSION="${DEF_VERSION}")
target_compile_definitions(xtcore_static  PRIVATE ${DEFS})
target_compile_definitions(xtcore_dynamic PRIVATE ${DEFS})
if (XTRACER_ENABLE_WASM)
    target_compile_definitions(${OUT_EXE_WASM} PRIVATE ${DEFS})
endif()
target_compile_definitions(${OUT_EXE_COLORSPACE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_COLORSPACE_VECTORS_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_TILE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_CONTEXT_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_SPHERE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_TRIANGLE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_WHITE_FURNACE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_RAYTRACER_EMISSIVE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_CLI_SETUP_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_NCF_PARSER_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_NMATH_BENCH} PRIVATE ${DEFS})
add_dependencies (${OUT_EXE_CLI} xtcore_static xtcore_dynamic manpages_cli)
add_dependencies (${OUT_EXE_COLORSPACE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_COLORSPACE_VECTORS_TEST} xtcore_static)
add_dependencies (${OUT_EXE_TILE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_CONTEXT_TEST} xtcore_static)
add_dependencies (${OUT_EXE_SPHERE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_TRIANGLE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_WHITE_FURNACE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_RAYTRACER_EMISSIVE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_CLI_SETUP_TEST} xtcore_static)
add_dependencies (${OUT_EXE_NCF_PARSER_TEST} xtcore_static)
add_dependencies (${OUT_EXE_NMATH_BENCH} xtcore_static)

enable_testing()

add_test(
    NAME colorspace::roundtrip
    COMMAND ${PATH_TEST}/${OUT_EXE_COLORSPACE_TEST}
)
add_test(
    NAME colorspace::vectors
    COMMAND ${PATH_TEST}/${OUT_EXE_COLORSPACE_VECTORS_TEST}
)
add_test(
    NAME xtcore::tile
    COMMAND ${PATH_TEST}/${OUT_EXE_TILE_TEST}
)
add_test(
    NAME xtcore::context
    COMMAND ${PATH_TEST}/${OUT_EXE_CONTEXT_TEST}
)
add_test(
    NAME xtcore::sphere
    COMMAND ${PATH_TEST}/${OUT_EXE_SPHERE_TEST}
)
add_test(
    NAME xtcore::triangle
    COMMAND ${PATH_TEST}/${OUT_EXE_TRIANGLE_TEST}
)
add_test(
    NAME xtcore::white_furnace
    COMMAND ${PATH_TEST}/${OUT_EXE_WHITE_FURNACE_TEST}
)
add_test(
    NAME xtcore::raytracer_emissive
    COMMAND ${PATH_TEST}/${OUT_EXE_RAYTRACER_EMISSIVE_TEST}
)
add_test(
    NAME cli::setup_parse
    COMMAND ${PATH_TEST}/${OUT_EXE_CLI_SETUP_TEST}
)
add_test(
    NAME ncf::inline_and_utf8
    COMMAND ${PATH_TEST}/${OUT_EXE_NCF_PARSER_TEST}
)
add_test(
    NAME cli::stencil_smoke
    COMMAND ${PATH_BIN}/${OUT_EXE_CLI} ${PATH_ROOT}/scene/lab-camera-modes-showcase.scn -renderer stencil -res 32x32 -aa 2 -samples 1 -rdepth 1 -outdir ${PATH_TEST}
)


install (TARGETS xtcore_static xtcore_dynamic
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install (TARGETS ${OUT_EXE_CLI}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install (FILES ${PATH_BIN}/${OUT_MAN_CLI} DESTINATION man/man1)
