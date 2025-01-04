#
# cmake build settings, modularized to be optionally included by parent projects
#

include_guard(GLOBAL)

macro(Setup)

message(STATUS "Setup: ${PROJECT_NAME}")

set(ENV{LANG} en_US.UTF-8)
set(ENV{LC_MEASUREMENT} en_US.UTF-8)

# Determine OS_AND_ARCH as library appendix, e.g. 'direct_bt-linux-amd64'
string(TOLOWER ${CMAKE_SYSTEM_NAME} OS_NAME)
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm")
    set(OS_ARCH "armhf")
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "armv7l")
    set(OS_ARCH "armhf")
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    set(OS_ARCH "arm64")
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    set(OS_ARCH "amd64")
else()
    set(OS_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()
set(OS_AND_ARCH ${OS_NAME}-${OS_ARCH})
set(os_and_arch_slash ${OS_NAME}/${OS_ARCH})
set(os_and_arch_dot ${OS_NAME}.${OS_ARCH})

message (STATUS "OS_NAME ${OS_NAME}")
message (STATUS "OS_ARCH ${OS_ARCH} (${CMAKE_SYSTEM_PROCESSOR})")
message (STATUS "OS_AND_ARCH ${OS_AND_ARCH}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(DEFINED CMAKE_CXX_CLANG_TIDY)
    message(STATUS "clang-tidy preset: ${CMAKE_CXX_CLANG_TIDY}")
else()
    message(STATUS "clang-tidy not used")
endif()

# for all
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set(cxx_clangd_flags "-pedantic, -pedantic-errors, -Wall, -Wextra, -Werror, -DDEBUG, -std=c++20")
message(STATUS "cxx_clangd_flags: ${cxx_clangd_flags}")

# for all
set (CC_FLAGS_WARNING "-Wall" "-Wextra" "-Werror")
set (GCC_FLAGS_WARNING_FORMAT "-Wformat=2" "-Wformat-overflow=2" "-Wformat-nonliteral" "-Wformat-security" "-Wformat-signedness" "-Wformat-y2k")
set (GCC_FLAGS_WARNING "-Wall" "-Wextra" "-Wshadow" "-Wtype-limits" "-Wsign-compare" "-Wcast-align=strict" "-Wnull-dereference" "-Winit-self" ${GCC_FLAGS_WARNING_FORMAT} "-Werror")
# causes issues in jau::get_int8(..): "-Wnull-dereference"
set (GCC_FLAGS_WARNING_NO_ERROR "-Wno-error=array-bounds" "-Wno-error=null-dereference" "-Wno-multichar")
set (CLANG_FLAGS_WARNING_NO_ERROR "-Wno-overloaded-virtual")

# too pedantic, but nice to check once in a while
# set (DISABLED_CC_FLAGS_WARNING "-Wsign-conversion")

# debug only
set (GCC_FLAGS_STACK "-fstack-protector-strong")
set (GCC_FLAGS_SANITIZE_ALLLEAK "-fsanitize-address-use-after-scope" "-fsanitize=address" "-fsanitize=pointer-compare" "-fsanitize=pointer-subtract" "-fsanitize=undefined" "-fsanitize=leak" "-fsanitize-recover=address")
set (GCC_FLAGS_SANITIZE_UNDEFINED "-fsanitize=undefined" "-fsanitize-recover=address")
set (GCC_FLAGS_SANITIZE_THREAD "-fsanitize-address-use-after-scope" "-fsanitize=undefined" "-fsanitize=thread" "-fsanitize-recover=address")
# -fsanitize=address cannot be combined with -fsanitize=thread
# -fsanitize=pointer-compare -fsanitize=pointer-subtract must be combined with -fsanitize=address
# -fsanitize=thread TSAN's lacks ability to properly handle GCC's atomic macros (like helgrind etc), can't check SC-DRF!

set(${PROJECT_NAME}_CXX_FLAGS ${CMAKE_CXX_FLAGS})
set(${PROJECT_NAME}_C_FLAGS ${CMAKE_C_FLAGS})
set(${PROJECT_NAME}_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
set(${PROJECT_NAME}_STATIC_LINKER_FLAGS ${CMAKE_STATIC_LINKER_FLAGS})
set(${PROJECT_NAME}_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})

if(CMAKE_COMPILER_IS_GNUCC)
    # shorten __FILE__ string and the like ..
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} ${GCC_FLAGS_WARNING} ${GCC_FLAGS_WARNING_NO_ERROR} "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=/")
else()
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} ${CC_FLAGS_WARNING} ${CLANG_FLAGS_WARNING_NO_ERROR})
endif(CMAKE_COMPILER_IS_GNUCC)

message(STATUS "${PROJECT_NAME} USE_STRIP = ${USE_STRIP} (pre-set)")

if(DEBUG)
    if(CMAKE_COMPILER_IS_GNUCC)
        set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} "-g" "-ggdb" "-DDEBUG" "-fno-omit-frame-pointer" ${GCC_FLAGS_STACK} "-no-pie")
    else()
        set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} "-g" "-DDEBUG")
    endif(CMAKE_COMPILER_IS_GNUCC)
    if(INSTRUMENTATION)
        if(CMAKE_COMPILER_IS_GNUCC)
            set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} ${GCC_FLAGS_SANITIZE_ALLLEAK})
        endif(CMAKE_COMPILER_IS_GNUCC)
    elseif(INSTRUMENTATION_UNDEFINED)
        if(CMAKE_COMPILER_IS_GNUCC)
            set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} ${GCC_FLAGS_SANITIZE_UNDEFINED})
        endif(CMAKE_COMPILER_IS_GNUCC)
    elseif(INSTRUMENTATION_THREAD)
        if(CMAKE_COMPILER_IS_GNUCC)
            set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} ${GCC_FLAGS_SANITIZE_THREAD})
        endif(CMAKE_COMPILER_IS_GNUCC)
    endif(INSTRUMENTATION)
elseif(GPROF)
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} "-O3" "-g" "-ggdb" "-pg")
elseif(PERF_ANALYSIS)
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} "-O3" "-g" "-ggdb")
else()
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} "-O3")
    find_program(STRIP strip)
    if (STRIP STREQUAL "STRIP-NOTFOUND")
        set(USE_STRIP OFF)
        message(STATUS "${PROJECT_NAME} USE_STRIP:=false, strip not found")
    elseif(NOT DEFINED USE_STRIP)
        set(USE_STRIP ON)
        message(STATUS "${PROJECT_NAME} USE_STRIP:=true, !DEBUG and not set")
    endif()
endif(DEBUG)

message(STATUS "${PROJECT_NAME} USE_STRIP = ${USE_STRIP} (final)")

if(DEBUG)
    if(CMAKE_COMPILER_IS_GNUCC)
        set(${PROJECT_NAME}_SHARED_LINKER_FLAGS  ${${PROJECT_NAME}_SHARED_LINKER_FLAGS} "-no-pie")
        set(${PROJECT_NAME}_EXE_LINKER_FLAGS  ${${PROJECT_NAME}_EXE_LINKER_FLAGS} "-no-pie")
    endif(CMAKE_COMPILER_IS_GNUCC)
else()
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} "-DNDEBUG")
endif(DEBUG)

if(DONT_USE_RTTI)
    message(STATUS "${PROJECT_NAME} RTTI disabled")
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} "-fno-rtti")
    set(${PROJECT_NAME}_SHARED_LINKER_FLAGS ${${PROJECT_NAME}_SHARED_LINKER_FLAGS} "-fno-rtti")
    #set(${PROJECT_NAME}_STATIC_LINKER_FLAGS ${${PROJECT_NAME}_STATIC_LINKER_FLAGS} "-fno-rtti")
    set(${PROJECT_NAME}_EXE_LINKER_FLAGS ${${PROJECT_NAME}_EXE_LINKER_FLAGS} "-fno-rtti")
else()
    message(STATUS "${PROJECT_NAME} RTTI enabled")
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} "-frtti")
    set(${PROJECT_NAME}_SHARED_LINKER_FLAGS ${${PROJECT_NAME}_SHARED_LINKER_FLAGS} "-frtti")
    #set(${PROJECT_NAME}_STATIC_LINKER_FLAGS ${${PROJECT_NAME}_STATIC_LINKER_FLAGS} "-frtti")
    set(${PROJECT_NAME}_EXE_LINKER_FLAGS ${${PROJECT_NAME}_EXE_LINKER_FLAGS} "-frtti")
endif(DONT_USE_RTTI)

if(NOT EMSCRIPTEN)
    if(${OS_NAME} STREQUAL "freebsd")
        set (SYS_INCLUDE_DIRS
          /usr/include
          /usr/local/include
        )
        set(CMAKE_SYSTEM_PREFIX_PATH "/usr;/usr/local")
    else()
        set (SYS_INCLUDE_DIRS
          /usr/include
        )
    endif()
    set(SDL2_LIBS "SDL2;SDL2_image;SDL2_ttf;SDL2_mixer")
else()
    # See https://emscripten.org/docs/tools_reference/settings_reference.html
    #
    # set(EMS_FLAGS "--use-port=sdl2" "--use-port=sdl2_image" "--use-port=sdl2_ttf" "--use-port=sdl2_mixer")
    set(EMS_FLAGS "SHELL:-s USE_SDL=2" "SHELL:-s USE_SDL_IMAGE=2" "SHELL:-s SDL2_IMAGE_FORMATS='[\"bmp\",\"png\"]'" "SHELL:-s USE_SDL_TTF=2" "SHELL:-s USE_SDL_MIXER=2" "-Wno-unused-command-line-argument")
    set(EMS_FLAGS ${EMS_FLAGS} "SHELL:-s WASM=1" "SHELL:-s LZ4=1" "SHELL:-s EXPORTED_RUNTIME_METHODS=cwrap")
    # set(EMS_FLAGS ${EMS_FLAGS} "SHELL:-s FULL_ES2=1") # would use client-side memory like FULL_ES3 -> bad performance
    set(EMS_FLAGS ${EMS_FLAGS} "SHELL:-s MAX_WEBGL_VERSION=2") # WebGL 2 -> ES3    
    set(EMS_FLAGS ${EMS_FLAGS} "SHELL:-s ALLOW_MEMORY_GROWTH=1")
    # set(EMS_FLAGS ${EMS_FLAGS} "-pthread") # fights w/ ALLOW_MEMORY_GROWTH
    # set(EMS_FLAGS ${EMS_FLAGS} "SHELL:-s MEMORY64=1") # wasm64 end-to-end: wasm32 object file can't be linked in wasm64 mode
    # set(EMS_FLAGS ${EMS_FLAGS} "SHELL:-s ASSERTIONS=1")
    set(EMS_FLAGS ${EMS_FLAGS} "SHELL:-s STACK_OVERFLOW_CHECK=1") # cheap cockie magic, enables CHECK_NULL_WRITES
    #
    # set(EMS_EXE_LD_FLAGS ${EMS_FLAGS}) "SHELL:-s SIDE_MODULE=1")
    set(EMS_EXE_LD_FLAGS ${EMS_FLAGS})
    set(EMS_STATIC_LD_FLAGS )
    message(STATUS "${PROJECT_NAME} EMS_FLAGS = ${EMS_FLAGS}")
    message(STATUS "${PROJECT_NAME} EMS_EXE_LD_FLAGS = ${EMS_EXE_LD_FLAGS}")
    message(STATUS "${PROJECT_NAME} EMS_STATIC_LD_FLAGS = ${EMS_STATIC_LD_FLAGS}")
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} ${EMS_FLAGS})
    set(${PROJECT_NAME}_C_FLAGS ${${PROJECT_NAME}_C_FLAGS} ${EMS_FLAGS})
    set(${PROJECT_NAME}_EXE_LINKER_FLAGS ${${PROJECT_NAME}_EXE_LINKER_FLAGS} ${EMS_EXE_LD_FLAGS})
    set(${PROJECT_NAME}_SHARED_LINKER_FLAGS ${${PROJECT_NAME}_SHARED_LINKER_FLAGS} ${EMS_FLAGS})
    set(${PROJECT_NAME}_STATIC_LINKER_FLAGS ${${PROJECT_NAME}_STATIC_LINKER_FLAGS} ${EMS_STATIC_LD_FLAGS})
    set(USE_SFML OFF)
    # set(DONT_USE_RTTI ON)
    set(USE_LIBUNWIND OFF)
    set(SDL2_LIBS "")
endif()

if(${OS_NAME} STREQUAL "linux")
    if(CMAKE_COMPILER_IS_GNUCC)
        set(CMAKE_CXX_STANDARD_LIBRARIES  "${CMAKE_CXX_STANDARD_LIBRARIES} -latomic")
    else()
        set(CMAKE_CXX_STANDARD_LIBRARIES  "${CMAKE_CXX_STANDARD_LIBRARIES} -latomic")
    endif(CMAKE_COMPILER_IS_GNUCC)
endif()

set (LIB_INSTALL_DIR "lib${LIB_SUFFIX}" CACHE PATH "Installation path for libraries")

message(STATUS "${PROJECT_NAME} ${PROJECT_NAME}_CXX_FLAGS = ${${PROJECT_NAME}_CXX_FLAGS}")
message(STATUS "${PROJECT_NAME} ${PROJECT_NAME}_SHARED_LINKER_FLAGS = ${${PROJECT_NAME}_SHARED_LINKER_FLAGS}")
message(STATUS "${PROJECT_NAME} ${PROJECT_NAME}_STATIC_LINKER_FLAGS = ${${PROJECT_NAME}_STATIC_LINKER_FLAGS}")
message(STATUS "${PROJECT_NAME} ${PROJECT_NAME}_EXE_LINKER_FLAGS = ${${PROJECT_NAME}_EXE_LINKER_FLAGS}")
message(STATUS "${PROJECT_NAME} CMAKE_CXX_STANDARD_LIBRARIES = ${CMAKE_CXX_STANDARD_LIBRARIES}")
message(STATUS "${PROJECT_NAME} LIB_INSTALL_DIR = ${LIB_INSTALL_DIR}")

# Set CMAKE_INSTALL_XXXDIR (XXX {BIN LIB ..} if not defined
# (was: CMAKE_LIB_INSTALL_DIR)
include(GNUInstallDirs)

# Appends the cmake/modules path to MAKE_MODULE_PATH variable.
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

message(STATUS "${PROJECT_NAME} USE_LIBCURL = ${USE_LIBCURL} (pre-set)")
if(NOT DEFINED USE_LIBCURL)
    set(USE_LIBCURL OFF)
    message(STATUS "${PROJECT_NAME} USE_LIBCURL ${USE_LIBCURL} (default)")
else()
    message(STATUS "${PROJECT_NAME} USE_LIBCURL ${USE_LIBCURL} (user)")
endif()
if(USE_LIBCURL)
    find_library(LIBCURL_LIBNAME "curl" REQUIRED)
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} "-DUSE_LIBCURL=1")
else()
    set(LIBCURL_LIBNAME "")
endif()
message(STATUS "${PROJECT_NAME} USE_LIBCURL ${USE_LIBCURL} -> libname ${LIBCURL_LIBNAME}")

message(STATUS "${PROJECT_NAME} USE_LIBUNWIND = ${USE_LIBUNWIND} (pre-set)")
if(NOT DEFINED USE_LIBUNWIND)
    set(USE_LIBUNWIND OFF)
    message(STATUS "${PROJECT_NAME} USE_LIBUNWIND ${USE_LIBUNWIND} (default)")
else()
    message(STATUS "${PROJECT_NAME} USE_LIBUNWIND ${USE_LIBUNWIND} (user)")
endif()
if(USE_LIBUNWIND)
    find_library(LIBUNWIND_LIBNAME "unwind" REQUIRED)
    set(${PROJECT_NAME}_CXX_FLAGS ${${PROJECT_NAME}_CXX_FLAGS} "-DUSE_LIBUNWIND=1")
else()
    set(LIBUNWIND_LIBNAME "")
endif()
message(STATUS "${PROJECT_NAME} USE_LIBUNWIND ${USE_LIBUNWIND} -> libname ${LIBUNWIND_LIBNAME}")

# Make a version file containing the current version from git.
include (GetGitRevisionDescription)

git_describe (VERSION "--tags")
get_git_head_revision(GIT_REFSPEC VERSION_SHA1 ALLOW_LOOKING_ABOVE_CMAKE_SOURCE_DIR)
git_local_changes(GIT_WORKDIR_DIRTY)
message(STATUS "${PROJECT_NAME} git_describe ${VERSION}")
message(STATUS "${PROJECT_NAME} get_git_head_revision ${GIT_REFSPEC}")
message(STATUS "${PROJECT_NAME} get_git_head_revision ${VERSION_SHA1}")
message(STATUS "${PROJECT_NAME} git_local_changes ${GIT_WORKDIR_DIRTY}")

if ("x_${VERSION}" STREQUAL "x_GIT-NOTFOUND" OR "x_${VERSION}" STREQUAL "x_HEAD-HASH-NOTFOUND" OR "x_${VERSION}" STREQUAL "x_-128-NOTFOUND")
  message (WARNING "Install git to compile for production!")
  set (VERSION "v1.0.0-dirty")
endif ()

message (STATUS "${PROJECT_NAME} version ${VERSION}")

#parse the version information into pieces.
string (REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
string (REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
string (REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION}")
string (REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+\\-([0-9]+).*" "\\1" VERSION_COMMITS "${VERSION}")
string (REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+\\-(.*)" "\\1" VERSION_SHA1_SHORT "${VERSION}")

if ("${VERSION_COMMITS}" MATCHES "^v.*")
  set (VERSION_COMMITS "")
endif()
if ("${VERSION_SHA1_SHORT}" MATCHES "^v.*")
  set (VERSION_SHA1_SHORT "")
endif()
if ("${GIT_WORKDIR_DIRTY}" STREQUAL "CLEAN")
    set (VERSION_GIT_DIRTY_SUFFIX "")
else()
    set (VERSION_GIT_DIRTY_SUFFIX "-dirty")
endif()
if ("${VERSION_COMMITS}" STREQUAL "")
  set (VERSION_LONG "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}${VERSION_GIT_DIRTY_SUFFIX}")
else ()
  set (VERSION_LONG "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_COMMITS}-${VERSION_SHA1_SHORT}${VERSION_GIT_DIRTY_SUFFIX}")
endif()
set (VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set (VERSION_API "${VERSION_MAJOR}.${VERSION_MINOR}")

if ("${GIT_WORKDIR_DIRTY}" STREQUAL "CLEAN")
    message (STATUS "${PROJECT_NAME} git repo is CLEAN")
else()
    message (STATUS "${PROJECT_NAME} git repo is DIRTY")
endif()
message (STATUS "${PROJECT_NAME} version major ${VERSION_MAJOR}, minor ${VERSION_MINOR}, patch ${VERSION_PATCH}, post version commits ${VERSION_COMMITS}, ssha1 ${VERSION_SHA1_SHORT}, sha1 ${VERSION_SHA1}")
message (STATUS "${PROJECT_NAME} VERSION_LONG  ${VERSION_LONG}")
message (STATUS "${PROJECT_NAME} VERSION_SHORT ${VERSION_SHORT}")
message (STATUS "${PROJECT_NAME} VERSION_API   ${VERSION_API}")

string(TIMESTAMP BUILD_TSTAMP "%Y-%m-%d %H:%M:%S")

endmacro()

