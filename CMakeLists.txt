cmake_minimum_required (VERSION 3.13.0 FATAL_ERROR)
project (gfxbox2 LANGUAGES C CXX)

if(${CMAKE_VERSION} VERSION_LESS "3.18.0")
    message(STATUS "CMake ${CMAKE_VERSION}, please consider to switch to CMake 3.18.0 or later")
else()
    message(STATUS "CMake ${CMAKE_VERSION} >= 3.18.0, good.")
endif()

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

# set(CMAKE_MESSAGE_LOG_LEVEL DEBUG)
# set(DEBUG ON)

if(NOT DEFINED DONT_USE_RTTI)
    set(DONT_USE_RTTI ON)
    message(STATUS "${PROJECT_NAME} DONT_USE_RTTI ${DONT_USE_RTTI} (gfxbox2 default)")
else()
    message(STATUS "${PROJECT_NAME} DONT_USE_RTTI ${DONT_USE_RTTI} (user)")
endif()

include(Setup.cmake)
Setup()

# Fixup clangd w/ absolute path
set (gfxbox2_clangd_source_args "-isystem, ${CMAKE_CURRENT_SOURCE_DIR}/include, -isystem, ${CMAKE_CURRENT_SOURCE_DIR}/src, -isystem, ${CMAKE_CURRENT_SOURCE_DIR}/examples, -isystem, ${CMAKE_CURRENT_SOURCE_DIR}/examples/funcdraw")
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/clangd.in
                ${CMAKE_CURRENT_BINARY_DIR}/.clangd)
file(COPY_FILE ${CMAKE_CURRENT_BINARY_DIR}/.clangd ${CMAKE_CURRENT_SOURCE_DIR}/.clangd)

# this is the library version, derived from git tag and revision
set (gfxbox2_VERSION_MAJOR      ${VERSION_MAJOR})
set (gfxbox2_VERSION_MINOR      ${VERSION_MINOR})
set (gfxbox2_VERSION_PATCH      ${VERSION_PATCH})
set (gfxbox2_VERSION_COMMITS    ${VERSION_COMMITS})
set (gfxbox2_VERSION_SHA1       ${VERSION_SHA1})
set (gfxbox2_VERSION_SHA1_SHORT ${VERSION_SHA1_SHORT})
set (gfxbox2_GIT_DIRTY          ${GIT_WORKDIR_DIRTY})
set (gfxbox2_VERSION_LONG       ${VERSION_LONG})
set (gfxbox2_VERSION_SHORT      ${VERSION_SHORT})
set (gfxbox2_VERSION_API        ${VERSION_API})

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/version.cpp.in
                ${CMAKE_CURRENT_BINARY_DIR}/src/version.cpp)

find_path (SYSTEM_USR_DIR "stdlib.h")
include_directories (${SYSTEM_USR_DIR})

add_subdirectory (src)
add_subdirectory (examples)
