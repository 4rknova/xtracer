cmake_minimum_required(VERSION 3.10)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are disabled. Use: cmake -S . -B build/intermediate/build")
endif()
project(xtracer)
include(GNUInstallDirs)

option(XTRACER_ENABLE_REMOTE_ASSETS "Enable remote URL asset fetching via curl subprocess" ON)
option(XTRACER_ENABLE_VIZ "Build sampling visualization tool (OpenGL/GLFW/GLEW)" OFF)
option(XTRACER_ENABLE_NMATH_SIMD "Enable SIMD paths in nmath (x86 SSE2, double precision)" ON)
option(XTRACER_ENABLE_NMATH_SIMD_AVX "Enable AVX path for nmath SIMD (requires XTRACER_ENABLE_NMATH_SIMD)" OFF)
option(XTRACER_ENABLE_OPENCL "Enable OpenCL GPU path tracer (experimental)" OFF)
set(XTRACER_NMATH_PERF_REPEATS "21" CACHE STRING "Repeat count for nmath_perf_check benchmark comparisons")

set(XTRACER_USING_EMSCRIPTEN OFF)
if (EMSCRIPTEN OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
    set(XTRACER_USING_EMSCRIPTEN ON)
endif()

if (XTRACER_ENABLE_NMATH_SIMD)
    if (XTRACER_USING_EMSCRIPTEN)
        message(WARNING "XTRACER_ENABLE_NMATH_SIMD is ignored for Emscripten builds")
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64|i[3-6]86|x86)$")
        add_compile_definitions(NMATH_ENABLE_SIMD=1)
        if (MSVC)
            if (XTRACER_ENABLE_NMATH_SIMD_AVX)
                add_compile_definitions(NMATH_ENABLE_SIMD_AVX=1)
                add_compile_options(/arch:AVX)
            else()
                add_compile_options(/arch:SSE2)
            endif()
        elseif (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            if (XTRACER_ENABLE_NMATH_SIMD_AVX)
                add_compile_definitions(NMATH_ENABLE_SIMD_AVX=1)
                add_compile_options(-mavx)
            else()
                add_compile_options(-msse2)
            endif()
        endif()
        if (XTRACER_ENABLE_NMATH_SIMD_AVX)
            message(STATUS "nmath SIMD enabled: x86 AVX double-precision paths")
        else()
            message(STATUS "nmath SIMD enabled: x86 SSE2 double-precision paths")
        endif()
    else()
        message(WARNING "XTRACER_ENABLE_NMATH_SIMD requested, but unsupported architecture '${CMAKE_SYSTEM_PROCESSOR}'; scalar nmath remains active")
    endif()
elseif (XTRACER_ENABLE_NMATH_SIMD_AVX)
    message(WARNING "XTRACER_ENABLE_NMATH_SIMD_AVX requires XTRACER_ENABLE_NMATH_SIMD=ON")
endif()

# PATHS
set(PATH_ROOT ${CMAKE_SOURCE_DIR})
set(PATH_SRC  ${PATH_ROOT}/src)
set(PATH_BIN ${CMAKE_BINARY_DIR})
set(PATH_WEB  ${PATH_SRC}/apps/web-client)
set(PATH_LIB  ${PATH_ROOT}/lib)
set(PATH_EXT  ${PATH_ROOT}/ext)
set(PATH_TEST ${PATH_BIN}/test)

# Ensure output directories exist for all build modes.
file(MAKE_DIRECTORY ${PATH_BIN})
file(MAKE_DIRECTORY ${PATH_TEST})

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

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(XTRACER_ENABLE_WEB_BUNDLE ON)
elseif (NOT DEFINED XTRACER_ENABLE_WEB_BUNDLE)
    set(XTRACER_ENABLE_WEB_BUNDLE OFF)
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

if (XTRACER_ENABLE_REMOTE_ASSETS AND NOT XTRACER_USING_EMSCRIPTEN)
    add_compile_definitions(XTCORE_ASSET_FETCHER_CURL)
    message(STATUS "Remote asset fetching: enabled (curl)")
else()
    message(STATUS "Remote asset fetching: disabled")
endif()

if (XTRACER_ENABLE_OPENCL)
    find_package(OpenCL REQUIRED)
    add_compile_definitions(XTCORE_ENABLE_OPENCL)
    message(STATUS "OpenCL GPU integrator: enabled (${OpenCL_VERSION_STRING})")
else()
    message(STATUS "OpenCL GPU integrator: disabled")
endif()

SCAN_DIRS(ext_libs)
list(APPEND ext_libs
    ${PATH_EXT}/pugixml
)
list(REMOVE_DUPLICATES ext_libs)

set(PATH_SRC_XTCORE       ${PATH_SRC}/xtcore)
set(PATH_SRC_FRONTEND_WEB ${PATH_SRC}/apps/xtracer)
set(PATH_SRC_BENCH        ${PATH_SRC}/bench)
set(PATH_SRC_TESTS        ${PATH_SRC}/tests)

set(IN_MAN  xtracer.1)

set(OUT_MAN xtracer.1.gz)
set(OUT_LIB     xtcore)
set(OUT_EXE_COLORSPACE_TEST test_nimg_colorspace)
set(OUT_EXE_COLORSPACE_VECTORS_TEST test_nimg_colorspace_vectors)
set(OUT_EXE_TILE_TEST test_xtcore_tile)
set(OUT_EXE_CONTEXT_TEST test_xtcore_context)
set(OUT_EXE_SPHERE_TEST test_xtcore_sphere)
set(OUT_EXE_TRIANGLE_TEST test_xtcore_triangle)
set(OUT_EXE_CSG_TEST test_xtcore_csg)
set(OUT_EXE_BOUNDARY_TEST test_xtcore_boundary_material)
set(OUT_EXE_WHITE_FURNACE_TEST test_xtcore_white_furnace)
set(OUT_EXE_RAYTRACER_EMISSIVE_TEST test_xtcore_raytracer_emissive)
set(OUT_EXE_OBJECT_MEDIUM_PARSE_TEST test_xtcore_object_medium_parse)
set(OUT_EXE_FBX_IMPORT_TEST test_xtcore_fbx_import)
set(OUT_EXE_GLTF_IMPORT_TEST test_xtcore_gltf_import)
set(OUT_EXE_SVG_MESH_GENERATOR_TEST test_xtcore_svg_mesh_generator)
set(OUT_EXE_NCF_PARSER_TEST test_ncf_parser)
set(OUT_EXE_SCENE_VALIDATOR_TEST test_xtcore_scene_validator)
set(OUT_EXE_SAMPLING_TEST test_nmath_sampling)
set(OUT_EXE_NMATH_SIMD_TEST test_nmath_simd)
set(OUT_EXE_NMATH_BENCH bench_nmath)
set(OUT_EXE_MESH_BENCH bench_mesh_intersection)
set(OUT_EXE_NMATH_BENCH_SIMD bench_nmath_simd)
set(OUT_EXE_NMATH_BENCH_AVX bench_nmath_avx)
set(OUT_EXE_NMATH_BENCH_SCALAR bench_nmath_scalar)
set(OUT_EXE_NMATH_SIMD_PERF_COMPARE test_nmath_simd_perf_compare)
set(OUT_EXE_CONVERT_MITSUBA convertMitsuba)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_FILES_DIRECTORY          ${PATH_BIN})
set(CMAKE_CACHEFILE_DIR            ${PATH_BIN})

file(GLOB SRC_LIBS
    ${PATH_LIB}/*/*.c
    ${PATH_LIB}/*/*.cc
    ${PATH_LIB}/*/*.cpp
)
set(SRC_EXT_UFBX
    ${PATH_EXT}/ufbx/ufbx.c
)
set(SRC_EXT_CGLTF
    ${PATH_EXT}/cgltf.c
)
set(SRC_NMATH_BENCH
    ${PATH_LIB}/nmath/vector.cc
    ${PATH_LIB}/nmath/matrix.cc
    ${PATH_LIB}/nmath/quadratic.cc
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

file(GLOB SRC_FRONTEND_WEB
    ${PATH_SRC_FRONTEND_WEB}/*.c
    ${PATH_SRC_FRONTEND_WEB}/*.cc
    ${PATH_SRC_FRONTEND_WEB}/*.cpp
)

add_custom_target(manpages
    COMMAND gzip -c ${CMAKE_SOURCE_DIR}/man/${IN_MAN} > ${PATH_BIN}/${OUT_MAN}
    VERBATIM
)

add_library(xtcore_static  STATIC ${SRC_XTCORE} ${SRC_LIBS} ${SRC_EXT_UFBX} ${SRC_EXT_CGLTF})
set_target_properties(xtcore_static  PROPERTIES OUTPUT_NAME xtcore)

add_library(xtcore_dynamic SHARED ${SRC_XTCORE} ${SRC_LIBS} ${SRC_EXT_UFBX} ${SRC_EXT_CGLTF})
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

if (XTRACER_ENABLE_OPENCL)
    set(OPENCL_KERNEL_SRC ${PATH_SRC_XTCORE}/integrator/gpu_opencl/pathtracer.cl)
    set(OPENCL_KERNEL_HDR ${CMAKE_BINARY_DIR}/pathtracer_cl_src.h)
    set(OPENCL_KERNEL_INCLUDES
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/sampler.cl
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/mat_lambert.cl
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/mat_dielectric.cl
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/mat_principled.cl
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/mat_subsurface.cl
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/mat_rough_dielectric.cl
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/mat_phong.cl
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/mat_sheen.cl
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/mat_thin_dielectric.cl
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/mat_thin_translucent.cl
        ${PATH_SRC_XTCORE}/integrator/gpu_opencl/mat_boundary.cl
    )
    add_custom_command(
        OUTPUT  ${OPENCL_KERNEL_HDR}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${OPENCL_KERNEL_SRC}
            -DOUTPUT_FILE=${OPENCL_KERNEL_HDR}
            -P ${PATH_SRC_XTCORE}/integrator/gpu_opencl/embed_kernel.cmake
        DEPENDS ${OPENCL_KERNEL_SRC}
                ${OPENCL_KERNEL_INCLUDES}
                ${PATH_SRC_XTCORE}/integrator/gpu_opencl/embed_kernel.cmake
        COMMENT "Embedding pathtracer.cl into pathtracer_cl_src.h"
        VERBATIM
    )
    add_custom_target(opencl_kernel_embed DEPENDS ${OPENCL_KERNEL_HDR})

    target_include_directories(xtcore_static PUBLIC ${CMAKE_BINARY_DIR})
    target_include_directories(xtcore_dynamic PUBLIC ${CMAKE_BINARY_DIR})
    target_link_libraries(xtcore_static  PUBLIC OpenCL::OpenCL)
    target_link_libraries(xtcore_dynamic PUBLIC OpenCL::OpenCL)
    add_dependencies(xtcore_static  opencl_kernel_embed)
    add_dependencies(xtcore_dynamic opencl_kernel_embed)
endif()

add_executable(${OUT_EXE_CONVERT_MITSUBA}
    ${PATH_SRC}/convertMitsuba/main.cc
    ${PATH_SRC}/convertMitsuba/converter.cc
    ${PATH_SRC}/convertMitsuba/packager.cc
)
add_executable(${OUT_EXE_COLORSPACE_TEST} ${PATH_SRC_TESTS}/colorspace_test.cc)
add_executable(${OUT_EXE_COLORSPACE_VECTORS_TEST} ${PATH_SRC_TESTS}/colorspace_vectors_test.cc)
add_executable(${OUT_EXE_TILE_TEST} ${PATH_SRC_TESTS}/tile_test.cc)
add_executable(${OUT_EXE_CONTEXT_TEST} ${PATH_SRC_TESTS}/context_test.cc)
add_executable(${OUT_EXE_SPHERE_TEST} ${PATH_SRC_TESTS}/sphere_test.cc)
add_executable(${OUT_EXE_TRIANGLE_TEST} ${PATH_SRC_TESTS}/triangle_test.cc)
add_executable(${OUT_EXE_CSG_TEST} ${PATH_SRC_TESTS}/csg_test.cc)
add_executable(${OUT_EXE_BOUNDARY_TEST} ${PATH_SRC_TESTS}/boundary_material_test.cc)
add_executable(${OUT_EXE_WHITE_FURNACE_TEST} ${PATH_SRC_TESTS}/white_furnace_test.cc)
add_executable(${OUT_EXE_RAYTRACER_EMISSIVE_TEST} ${PATH_SRC_TESTS}/raytracer_emissive_test.cc)
add_executable(${OUT_EXE_OBJECT_MEDIUM_PARSE_TEST} ${PATH_SRC_TESTS}/object_medium_parse_test.cc)
add_executable(${OUT_EXE_FBX_IMPORT_TEST} ${PATH_SRC_TESTS}/fbx_import_test.cc)
add_executable(${OUT_EXE_GLTF_IMPORT_TEST} ${PATH_SRC_TESTS}/gltf_import_test.cc)
add_executable(${OUT_EXE_SVG_MESH_GENERATOR_TEST} ${PATH_SRC_TESTS}/svg_mesh_generator_test.cc)
add_executable(${OUT_EXE_NCF_PARSER_TEST} ${PATH_SRC_TESTS}/ncf_parser_test.cc)
add_executable(${OUT_EXE_SCENE_VALIDATOR_TEST} ${PATH_SRC_TESTS}/scene_validator_test.cc)
add_executable(${OUT_EXE_SAMPLING_TEST} ${PATH_SRC_TESTS}/sampling_test.cc)
add_executable(${OUT_EXE_NMATH_SIMD_TEST} ${PATH_SRC_TESTS}/nmath_simd_test.cc)
add_executable(${OUT_EXE_NMATH_BENCH} ${PATH_SRC_TESTS}/nmath_bench.cc)
add_executable(${OUT_EXE_MESH_BENCH} ${PATH_SRC_BENCH}/mesh_intersection_bench.cc)
add_executable(${OUT_EXE_NMATH_BENCH_SIMD} ${PATH_SRC_TESTS}/nmath_bench.cc ${SRC_NMATH_BENCH})
add_executable(${OUT_EXE_NMATH_BENCH_AVX} ${PATH_SRC_TESTS}/nmath_bench.cc ${SRC_NMATH_BENCH})
add_executable(${OUT_EXE_NMATH_BENCH_SCALAR} ${PATH_SRC_TESTS}/nmath_bench.cc ${SRC_NMATH_BENCH})
add_executable(${OUT_EXE_NMATH_SIMD_PERF_COMPARE} ${PATH_SRC_TESTS}/nmath_simd_perf_compare_test.cc)
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
set_target_properties(${OUT_EXE_CSG_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_BOUNDARY_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_WHITE_FURNACE_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_RAYTRACER_EMISSIVE_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_OBJECT_MEDIUM_PARSE_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_FBX_IMPORT_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_GLTF_IMPORT_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_SVG_MESH_GENERATOR_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_NCF_PARSER_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_SCENE_VALIDATOR_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_SAMPLING_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_NMATH_SIMD_TEST} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)
set_target_properties(${OUT_EXE_NMATH_BENCH} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN}
)
set_target_properties(${OUT_EXE_MESH_BENCH} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN}
)
set_target_properties(${OUT_EXE_NMATH_BENCH_SIMD} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN}
)
set_target_properties(${OUT_EXE_NMATH_BENCH_AVX} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN}
)
set_target_properties(${OUT_EXE_NMATH_BENCH_SCALAR} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN}
)
set_target_properties(${OUT_EXE_NMATH_SIMD_PERF_COMPARE} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_TEST}
)

target_include_directories(${OUT_EXE_CONVERT_MITSUBA} PRIVATE
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_CONVERT_MITSUBA} SYSTEM PRIVATE
    ${PATH_EXT}/pugixml
)
target_compile_options(${OUT_EXE_CONVERT_MITSUBA} PRIVATE -std=c++17)
set_target_properties(${OUT_EXE_CONVERT_MITSUBA} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN}
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
target_include_directories(${OUT_EXE_CSG_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_CSG_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_BOUNDARY_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_BOUNDARY_TEST} SYSTEM PUBLIC
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
target_include_directories(${OUT_EXE_OBJECT_MEDIUM_PARSE_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_OBJECT_MEDIUM_PARSE_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_FBX_IMPORT_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_FBX_IMPORT_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_GLTF_IMPORT_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_GLTF_IMPORT_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_SVG_MESH_GENERATOR_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_SVG_MESH_GENERATOR_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_NCF_PARSER_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_NCF_PARSER_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_SCENE_VALIDATOR_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_SCENE_VALIDATOR_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_SAMPLING_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_SAMPLING_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_NMATH_SIMD_TEST} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_NMATH_SIMD_TEST} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_NMATH_BENCH} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_NMATH_BENCH} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_MESH_BENCH} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_MESH_BENCH} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_NMATH_BENCH_SIMD} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_NMATH_BENCH_SIMD} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_NMATH_BENCH_AVX} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_NMATH_BENCH_AVX} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_NMATH_BENCH_SCALAR} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_NMATH_BENCH_SCALAR} SYSTEM PUBLIC
    ${ext_libs}
)
target_include_directories(${OUT_EXE_NMATH_SIMD_PERF_COMPARE} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_NMATH_SIMD_PERF_COMPARE} SYSTEM PUBLIC
    ${ext_libs}
)

target_link_libraries(${OUT_EXE_COLORSPACE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_COLORSPACE_VECTORS_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_TILE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_CONTEXT_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_SPHERE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_TRIANGLE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_CSG_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_BOUNDARY_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_WHITE_FURNACE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_RAYTRACER_EMISSIVE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_OBJECT_MEDIUM_PARSE_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_FBX_IMPORT_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_GLTF_IMPORT_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_SVG_MESH_GENERATOR_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_NCF_PARSER_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_SCENE_VALIDATOR_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_SAMPLING_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_NMATH_SIMD_TEST} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_NMATH_BENCH} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_MESH_BENCH} PRIVATE xtcore_static pthread)
target_link_libraries(${OUT_EXE_NMATH_BENCH_SIMD} PRIVATE pthread)
target_link_libraries(${OUT_EXE_NMATH_BENCH_AVX} PRIVATE pthread)
target_link_libraries(${OUT_EXE_NMATH_BENCH_SCALAR} PRIVATE pthread)
target_link_libraries(${OUT_EXE_NMATH_SIMD_PERF_COMPARE} PRIVATE pthread)

target_compile_definitions(${OUT_EXE_NMATH_BENCH_SIMD} PRIVATE NMATH_ENABLE_SIMD=1)
if (MSVC)
    target_compile_options(${OUT_EXE_NMATH_BENCH_SIMD} PRIVATE /UNMATH_ENABLE_SIMD_AVX)
else()
    target_compile_options(${OUT_EXE_NMATH_BENCH_SIMD} PRIVATE -UNMATH_ENABLE_SIMD_AVX -mno-avx)
endif()
target_compile_definitions(${OUT_EXE_NMATH_BENCH_AVX} PRIVATE NMATH_ENABLE_SIMD=1 NMATH_ENABLE_SIMD_AVX=1)
if (MSVC)
    target_compile_options(${OUT_EXE_NMATH_BENCH_AVX} PRIVATE /arch:AVX)
else()
    target_compile_options(${OUT_EXE_NMATH_BENCH_AVX} PRIVATE -mavx)
endif()
target_compile_definitions(${OUT_EXE_NMATH_BENCH_SCALAR} PRIVATE NMATH_BENCH_FORCE_SCALAR=1)

set(OUT_EXE_WEB xtracer)
add_executable(${OUT_EXE_WEB} ${SRC_FRONTEND_WEB})
target_include_directories(${OUT_EXE_WEB} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
)
target_include_directories(${OUT_EXE_WEB} SYSTEM PUBLIC
    ${ext_libs}
)
# Asio is provided by libasio-dev (system package).
# Do NOT define CROW_USE_BOOST — crow checks #ifdef CROW_USE_BOOST, so any
# definition (including =0) triggers Boost. Just define ASIO_STANDALONE.
target_compile_definitions(${OUT_EXE_WEB} PRIVATE
    ASIO_STANDALONE
)
target_link_libraries(${OUT_EXE_WEB} PRIVATE xtcore_static pthread)
add_dependencies(${OUT_EXE_WEB} web_license_asset manpages)

if (XTRACER_ENABLE_WEB_BUNDLE)
    find_program(NODE_EXECUTABLE node REQUIRED)
    add_custom_target(web_bundle
        COMMAND ${NODE_EXECUTABLE} ${PATH_WEB}/build.mjs --web-root ${PATH_WEB}
        WORKING_DIRECTORY ${PATH_WEB}
        COMMENT "Bundling web-client assets"
        VERBATIM
    )
    add_dependencies(${OUT_EXE_WEB} web_bundle)
    target_compile_definitions(${OUT_EXE_WEB} PRIVATE XTRACER_WEB_BUNDLE=1)
    message(STATUS "Web client bundle: ON")
else()
    message(STATUS "Web client bundle: OFF (XTRACER_ENABLE_WEB_BUNDLE=ON to enable)")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wno-unknown-pragmas")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(DEFS ${DEFS} XTCORE_VERSION="${DEF_VERSION}")
target_compile_definitions(xtcore_static  PRIVATE ${DEFS})
target_compile_definitions(xtcore_dynamic PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_COLORSPACE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_COLORSPACE_VECTORS_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_TILE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_CONTEXT_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_SPHERE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_TRIANGLE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_CSG_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_BOUNDARY_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_WHITE_FURNACE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_RAYTRACER_EMISSIVE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_OBJECT_MEDIUM_PARSE_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_SVG_MESH_GENERATOR_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_NCF_PARSER_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_SCENE_VALIDATOR_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_SAMPLING_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_NMATH_SIMD_TEST} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_NMATH_BENCH} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_MESH_BENCH} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_NMATH_BENCH_SIMD} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_NMATH_BENCH_AVX} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_NMATH_BENCH_SCALAR} PRIVATE ${DEFS})
target_compile_definitions(${OUT_EXE_NMATH_SIMD_PERF_COMPARE} PRIVATE ${DEFS})
add_dependencies (${OUT_EXE_COLORSPACE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_COLORSPACE_VECTORS_TEST} xtcore_static)
add_dependencies (${OUT_EXE_TILE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_CONTEXT_TEST} xtcore_static)
add_dependencies (${OUT_EXE_SPHERE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_TRIANGLE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_CSG_TEST} xtcore_static)
add_dependencies (${OUT_EXE_BOUNDARY_TEST} xtcore_static)
add_dependencies (${OUT_EXE_WHITE_FURNACE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_RAYTRACER_EMISSIVE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_OBJECT_MEDIUM_PARSE_TEST} xtcore_static)
add_dependencies (${OUT_EXE_FBX_IMPORT_TEST} xtcore_static)
add_dependencies (${OUT_EXE_GLTF_IMPORT_TEST} xtcore_static)
add_dependencies (${OUT_EXE_SVG_MESH_GENERATOR_TEST} xtcore_static)
add_dependencies (${OUT_EXE_NCF_PARSER_TEST} xtcore_static)
add_dependencies (${OUT_EXE_SCENE_VALIDATOR_TEST} xtcore_static)
add_dependencies (${OUT_EXE_SAMPLING_TEST} xtcore_static)
add_dependencies (${OUT_EXE_NMATH_SIMD_TEST} xtcore_static)
add_dependencies (${OUT_EXE_NMATH_BENCH} xtcore_static)
add_dependencies (${OUT_EXE_MESH_BENCH} xtcore_static)
add_dependencies (${OUT_EXE_NMATH_SIMD_PERF_COMPARE} ${OUT_EXE_NMATH_BENCH_SIMD} ${OUT_EXE_NMATH_BENCH_AVX} ${OUT_EXE_NMATH_BENCH_SCALAR})

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS
        ${OUT_EXE_COLORSPACE_TEST}
        ${OUT_EXE_COLORSPACE_VECTORS_TEST}
        ${OUT_EXE_TILE_TEST}
        ${OUT_EXE_CONTEXT_TEST}
        ${OUT_EXE_SPHERE_TEST}
        ${OUT_EXE_TRIANGLE_TEST}
        ${OUT_EXE_CSG_TEST}
        ${OUT_EXE_BOUNDARY_TEST}
        ${OUT_EXE_WHITE_FURNACE_TEST}
        ${OUT_EXE_RAYTRACER_EMISSIVE_TEST}
        ${OUT_EXE_OBJECT_MEDIUM_PARSE_TEST}
        ${OUT_EXE_FBX_IMPORT_TEST}
        ${OUT_EXE_GLTF_IMPORT_TEST}
        ${OUT_EXE_SVG_MESH_GENERATOR_TEST}
        ${OUT_EXE_NCF_PARSER_TEST}
        ${OUT_EXE_SCENE_VALIDATOR_TEST}
        ${OUT_EXE_SAMPLING_TEST}
        ${OUT_EXE_NMATH_SIMD_TEST}
        ${OUT_EXE_NMATH_SIMD_PERF_COMPARE}
)

add_custom_target(nmath_perf_check
    COMMAND ${CMAKE_CTEST_COMMAND} -V --output-on-failure -R nmath::simd_perf_compare
    COMMAND ${CMAKE_COMMAND} -E echo "nmath_perf_check uses repeats=${XTRACER_NMATH_PERF_REPEATS}"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS
        ${OUT_EXE_NMATH_SIMD_PERF_COMPARE}
        ${OUT_EXE_NMATH_BENCH_SIMD}
        ${OUT_EXE_NMATH_BENCH_AVX}
        ${OUT_EXE_NMATH_BENCH_SCALAR}
)

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
    NAME xtcore::csg
    COMMAND ${PATH_TEST}/${OUT_EXE_CSG_TEST}
)
add_test(
    NAME xtcore::boundary_material
    COMMAND ${PATH_TEST}/${OUT_EXE_BOUNDARY_TEST}
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
    NAME xtcore::object_medium_parse
    COMMAND ${PATH_TEST}/${OUT_EXE_OBJECT_MEDIUM_PARSE_TEST}
)
add_test(
    NAME xtcore::fbx_import
    COMMAND ${PATH_TEST}/${OUT_EXE_FBX_IMPORT_TEST}
)
add_test(
    NAME xtcore::gltf_import
    COMMAND ${PATH_TEST}/${OUT_EXE_GLTF_IMPORT_TEST}
)
add_test(
    NAME xtcore::svg_mesh_generator
    COMMAND ${PATH_TEST}/${OUT_EXE_SVG_MESH_GENERATOR_TEST}
)
add_test(
    NAME ncf::inline_and_utf8
    COMMAND ${PATH_TEST}/${OUT_EXE_NCF_PARSER_TEST}
)
add_test(
    NAME scene::validate_all
    COMMAND ${PATH_TEST}/${OUT_EXE_SCENE_VALIDATOR_TEST}
)
add_test(
    NAME nmath::sampling
    COMMAND ${PATH_TEST}/${OUT_EXE_SAMPLING_TEST}
)
add_test(
    NAME nmath::simd
    COMMAND ${PATH_TEST}/${OUT_EXE_NMATH_SIMD_TEST}
)
add_test(
    NAME nmath::simd_perf_compare
    COMMAND ${PATH_TEST}/${OUT_EXE_NMATH_SIMD_PERF_COMPARE} ${PATH_BIN}/${OUT_EXE_NMATH_BENCH_SIMD} ${PATH_BIN}/${OUT_EXE_NMATH_BENCH_AVX} ${PATH_BIN}/${OUT_EXE_NMATH_BENCH_SCALAR} --iters 1200000 --repeats ${XTRACER_NMATH_PERF_REPEATS}
)
install (TARGETS xtcore_static xtcore_dynamic
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install (TARGETS ${OUT_EXE_WEB}
    RUNTIME DESTINATION bin
)

install (TARGETS ${OUT_EXE_CONVERT_MITSUBA}
    RUNTIME DESTINATION bin
)

install (FILES ${PATH_BIN}/${OUT_MAN} DESTINATION man/man1)

if (UNIX)
    install (FILES ${PATH_ROOT}/src/util/bash/autocomplete/xtcli
        DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
        RENAME xtracer)
endif()
