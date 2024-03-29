cmake_minimum_required(VERSION 3.1)
project(xtracer)

set(OpenGL_GL_PREFERENCE GLVND)

find_package(OpenGL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.2 REQUIRED)
find_package(OpenMP REQUIRED)

# PATHS
set(PATH_ROOT ${CMAKE_SOURCE_DIR})
set(PATH_SRC  ${PATH_ROOT}/src)
set(PATH_BIN  ${PATH_ROOT}/bin)
set(PATH_LIB  ${PATH_ROOT}/lib)
set(PATH_EXT  ${PATH_ROOT}/ext)
set(PATH_TEST ${PATH_BIN}/test)

set(PATH_SRC_XTCORE       ${PATH_SRC}/xtcore)
set(PATH_SRC_FRONTEND_CLI ${PATH_SRC}/frontend/cli)
set(PATH_SRC_FRONTEND_GUI ${PATH_SRC}/frontend/gui)

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

SCAN_DIRS(ext_libs)

set(IN_MAN_CLI  xtracer_cli.1)

set(OUT_MAN_CLI xtracer_cli.1.gz)
set(OUT_LIB     xtcore)
set(OUT_EXE_CLI xtracer_cli)
set(OUT_EXE_GUI xtracer_gui)

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

file(GLOB SRC_FRONTEND_GUI
    ${PATH_SRC_FRONTEND_GUI}/*.c
    ${PATH_SRC_FRONTEND_GUI}/*.cc
    ${PATH_SRC_FRONTEND_GUI}/*.cpp
    ${PATH_SRC_FRONTEND_GUI}/*/*.c
    ${PATH_SRC_FRONTEND_GUI}/*/*.cc
    ${PATH_SRC_FRONTEND_GUI}/*/*.cpp
)

file(GLOB SRC_IMGUI
    ${PATH_EXT}/imgui/*.cpp
)

file(GLOB SRC_REMOTERY
    ${PATH_EXT}/remotery/lib/*.c
)

file(GLOB SRC_RTMIDI
    ${PATH_EXT}/rtmidi/*.cpp
)

add_custom_target(manpages_cli
    gzip -c man/${IN_MAN_CLI} > ${PATH_BIN}/${OUT_MAN_CLI}
)

add_library(imgui STATIC ${SRC_IMGUI})
set_target_properties(imgui PROPERTIES OUTPUT_NAME imgui)

add_library(remotery STATIC ${SRC_REMOTERY})
set_target_properties(remotery PROPERTIES OUTPUT_NAME remotery)
set_target_properties(remotery PROPERTIES COMPILE_FLAGS "-std=gnu99 -I{PATH_EXT}/remotery/lib/ -pthread -lm")

add_library(rtmidi STATIC ${SRC_RTMIDI})
set_target_properties(rtmidi PROPERTIES OUTPUT_NAME rtmidi)
set_target_properties(rtmidi PROPERTIES COMPILE_FLAGS "-I{PATH_EXT}/rtmidi/ -D__LINUX_ALSA__")

add_library(xtcore_static  STATIC ${SRC_XTCORE} ${SRC_LIBS})
set_target_properties(xtcore_static  PROPERTIES OUTPUT_NAME xtcore)

add_library(xtcore_dynamic SHARED ${SRC_XTCORE} ${SRC_LIBS})
set_target_properties(xtcore_dynamic PROPERTIES OUTPUT_NAME xtcore)

target_include_directories(xtcore_static PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
    ${ext_libs}
)

target_include_directories(xtcore_dynamic PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
    ${ext_libs}
)

add_executable(${OUT_EXE_CLI} ${SRC_FRONTEND_CLI})
add_executable(${OUT_EXE_GUI} ${SRC_FRONTEND_GUI})

target_include_directories(${OUT_EXE_CLI} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
    ${ext_libs}
)

target_include_directories(${OUT_EXE_GUI} PUBLIC
    ${PATH_LIB}
    ${PATH_SRC}
    ${ext_libs}
)

target_link_libraries(${OUT_EXE_CLI} PRIVATE xtcore_static remotery rtmidi asound pthread)
target_link_libraries(${OUT_EXE_GUI} PRIVATE xtcore_static remotery rtmidi asound pthread imgui ssl crypto GL glfw GLEW::GLEW ${OpenMP_LIBRARY})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wno-unknown-pragmas")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O0")

set(DEFS ${DEFS} XTCORE_VERSION="${DEF_VERSION}")
target_compile_definitions(xtcore_static  PRIVATE ${DEFS})
target_compile_definitions(xtcore_dynamic PRIVATE ${DEFS})
add_dependencies (${OUT_EXE_CLI} xtcore_static xtcore_dynamic manpages_cli)
add_dependencies (${OUT_EXE_GUI} xtcore_static xtcore_dynamic)

enable_testing()
file(MAKE_DIRECTORY ${PATH_TEST})
file(GLOB files "scene/*.scn")
foreach(file ${files})
    get_filename_component(fname ${file} NAME)
    add_test(
        NAME render::${fname}-stencil
        COMMAND ${PATH_BIN}/${OUT_EXE_CLI} ${file} -outdir ${PATH_TEST} -renderer stencil
    )
endforeach()


install (TARGETS ${OUT_EXE_CLI} ${OUT_EXE_GUI} xtcore_static xtcore_dynamic
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install (FILES ${PATH_BIN}/${OUT_MAN_CLI} DESTINATION man/man1)
