cmake_minimum_required(VERSION 2.8)
project(xtracer)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenMP REQUIRED)

execute_process(
    COMMAND date +"%Y"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE DEF_YEAR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND git describe --long
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE DEF_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(PATH_ROOT ${CMAKE_SOURCE_DIR})
set(PATH_SRC  ${PATH_ROOT}/src)
set(PATH_BIN  ${PATH_ROOT}/bin)
set(PATH_LIB  ${PATH_ROOT}/lib)
set(PATH_EXT  ${PATH_ROOT}/ext)
set(PATH_TEST ${PATH_BIN}/test)

set(OUT_LIB     xtcore)
set(OUT_EXE_CLI xtracer_cli)
set(OUT_EXE_GUI xtracer_gui)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PATH_BIN})
set(CMAKE_FILES_DIRECTORY          ${PATH_BIN})
set(CMAKE_CACHEFILE_DIR ${PATH_BIN})

set(PATH_SRC_XTCORE ${PATH_SRC}/xtcore)
set(PATH_SRC_FRONTEND_CLI ${PATH_SRC}/xt_cli)
set(PATH_SRC_FRONTEND_GUI ${PATH_SRC}/xt_gui)

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
)

file(GLOB SRC_FRONTEND_CLI
    ${PATH_SRC_FRONTEND_CLI}/*.c
    ${PATH_SRC_FRONTEND_CLI}/*.cc
    ${PATH_SRC_FRONTEND_CLI}/*.cpp
)

file(GLOB SRC_FRONTEND_GUI
    ${PATH_SRC_FRONTEND_GUI}/*.c
    ${PATH_SRC_FRONTEND_GUI}/*.cc
    ${PATH_SRC_FRONTEND_GUI}/*.cpp
)

add_library(xtcore_static  STATIC ${SRC_XTCORE} ${SRC_LIBS})
add_library(xtcore_dynamic SHARED ${SRC_XTCORE} ${SRC_LIBS})
set_target_properties(xtcore_static  PROPERTIES OUTPUT_NAME xtcore)
set_target_properties(xtcore_dynamic PROPERTIES OUTPUT_NAME xtcore)

target_include_directories(xtcore_static PUBLIC
    ${PATH_LIB}
    ${PATH_EXT}
)

target_include_directories(xtcore_dynamic PUBLIC
    ${PATH_LIB}
    ${PATH_EXT}
)

add_executable(${OUT_EXE_CLI} ${SRC_FRONTEND_CLI})
add_executable(${OUT_EXE_GUI} ${SRC_FRONTEND_GUI})

target_include_directories(${OUT_EXE_CLI} PUBLIC
    ${PATH_LIB}
    ${PATH_EXT}
    ${PATH_SRC}
)

target_include_directories(${OUT_EXE_GUI} PUBLIC
    ${PATH_LIB}
    ${PATH_EXT}
    ${PATH_SRC}
)

target_link_libraries(${OUT_EXE_CLI} PRIVATE "${PATH_BIN}/libxtcore.a")

SET(LIBRARIES ${OPENGL_LIBRARY} ${GLUT_LIBRARY} ${GLEW_LIBRARY} ${OpenMP_LIBRARY} ${CMAKE_DL_LIBS})

target_link_libraries(${OUT_EXE_GUI} PRIVATE "${PATH_BIN}/libxtcore.a" ${LIBRARIES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

target_compile_definitions(xtracer_cli PRIVATE XTRACER_YEAR=""${DEF_YEAR}"" XTRACER_VERSION="${DEF_VERSION}")

enable_testing()
file(MAKE_DIRECTORY ${PATH_TEST})
file(GLOB files "scene/*.scn")
foreach(file ${files})
    get_filename_component(fname ${file} NAME)
    add_test(
        NAME render::${fname}-stencil
        COMMAND ${PATH_BIN}/${OUT_EXE_CLI} ${file} -outdir ${PATH_TEST} -renderer stencil
    )
    add_test(
        NAME render::${fname}-depth
        COMMAND ${PATH_BIN}/${OUT_EXE_CLI} ${file} -outdir ${PATH_TEST} -renderer depth
    )
    add_test(
        NAME render::${fname}
        COMMAND ${PATH_BIN}/${OUT_EXE_CLI} ${file} -outdir ${PATH_TEST}
    )
endforeach()