
include(carbin_print)
include(carbin_cc_library)
include(carbin_cc_test)
include(carbin_cc_binary)
include(carbin_cc_benchmark)
include(carbin_print_list)
include(carbin_check)
include(carbin_variable)
include(carbin_include)

if (NOT $ENV{CONDA_PREFIX} STREQUAL "")
    carbin_print("set up conda env")
    list(APPEND CMAKE_PREFIX_PATH $ENV{CONDA_PREFIX})
    carbin_include("$ENV{CONDA_PREFIX}/${CARBIN_INSTALL_INCLUDEDIR}")
    if (APPLE)
        set(CMAKE_CXX_COMPILER $ENV{CONDA_PREFIX}/bin/clang++)
        set(CMAKE_C_COMPILER $ENV{CONDA_PREFIX}/bin/clang)
    endif ()
endif ()

carbin_include("${PROJECT_BINARY_DIR}")
carbin_include("${PROJECT_SOURCE_DIR}")

MACRO(directory_list result curdir)
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    SET(dirlist "")
    FOREACH (child ${children})
        IF (IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        ENDIF ()
    ENDFOREACH ()
    SET(${result} ${dirlist})
ENDMACRO()

directory_list(install_libs "${CARBIN_PREFIX}/${CARBIN_INSTALL_LIBDIR}/cmake")

FOREACH (installed_lib ${install_libs})
    list(APPEND CMAKE_MODULE_PATH
            ${CARBIN_PREFIX}/${CARBIN_INSTALL_LIBDIR}/cmake/${installed_lib})

ENDFOREACH ()

include(carbin_outof_source)
include(carbin_platform)
#include(carbin_pkg_dump)

CARBIN_ENSURE_OUT_OF_SOURCE_BUILD("must out of source dir")

#if (NOT DEV_MODE AND ${PROJECT_VERSION} MATCHES "0.0.0")
#    carbin_error("PROJECT_VERSION must be set in file project_profile or set -DDEV_MODE=true for develop debug")
#endif()


set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # require at least gcc 4.8
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.3)
        message(FATAL_ERROR "GCC is too old, please install a newer version supporting C++17")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # require at least clang 3.3
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.3)
        message(FATAL_ERROR "Clang is too old, please install a newer version supporting C++17")
    endif ()
else ()
    message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC. " ${CMAKE_CXX_COMPILER_ID})
endif ()

#####
# get project version

execute_process(
        COMMAND bash -c "${PROJECT_SOURCE_DIR}/tools/get_flare_revision.sh ${PROJECT_SOURCE_DIR} | tr -d '\n'"
        OUTPUT_VARIABLE CARBIN_PROJECT_REVISION
)

