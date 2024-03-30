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

if(DEFINED CMAKE_CXX_CLANG_TIDY)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message(STATUS "clang-tidy preset: ${CMAKE_CXX_CLANG_TIDY}")
else()
    message(STATUS "clang-tidy not used")
endif()

# for all
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# for all
set (CC_FLAGS_WARNING "-Wall -Wextra -Werror")
set (GCC_FLAGS_WARNING_FORMAT "-Wformat=2 -Wformat-overflow=2 -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat-y2k")
set (GCC_FLAGS_WARNING "-Wall -Wextra -Wshadow -Wtype-limits -Wsign-compare -Wcast-align=strict -Wnull-dereference -Winit-self ${GCC_FLAGS_WARNING_FORMAT} -Werror")
# causes issues in jau::get_int8(..): "-Wnull-dereference"
set (GCC_FLAGS_WARNING_NO_ERROR "-Wno-error=array-bounds -Wno-error=null-dereference -Wno-multichar")
set (CLANG_FLAGS_WARNING_NO_ERROR "-Wno-overloaded-virtual")

# too pedantic, but nice to check once in a while
# set (DISABLED_CC_FLAGS_WARNING "-Wsign-conversion")

# debug only
set (GCC_FLAGS_STACK "-fstack-protector-strong")
set (GCC_FLAGS_SANITIZE_ALLLEAK "-fsanitize-address-use-after-scope -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=undefined -fsanitize=leak -fsanitize-recover=address")
set (GCC_FLAGS_SANITIZE_UNDEFINED "-fsanitize=undefined -fsanitize-recover=address")
set (GCC_FLAGS_SANITIZE_THREAD "-fsanitize-address-use-after-scope -fsanitize=undefined -fsanitize=thread -fsanitize-recover=address")
# -fsanitize=address cannot be combined with -fsanitize=thread
# -fsanitize=pointer-compare -fsanitize=pointer-subtract must be combined with -fsanitize=address
# -fsanitize=thread TSAN's lacks ability to properly handle GCC's atomic macros (like helgrind etc), can't check SC-DRF!

if(CMAKE_COMPILER_IS_GNUCC)
    # shorten __FILE__ string and the like ..
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_FLAGS_WARNING} ${GCC_FLAGS_WARNING_NO_ERROR} -fmacro-prefix-map=${CMAKE_SOURCE_DIR}/=/")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CC_FLAGS_WARNING} ${CLANG_FLAGS_WARNING_NO_ERROR}")
endif(CMAKE_COMPILER_IS_GNUCC)

message(STATUS "${PROJECT_NAME} USE_STRIP = ${USE_STRIP} (pre-set)")

if(DEBUG)
    if(CMAKE_COMPILER_IS_GNUCC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb -DDEBUG -fno-omit-frame-pointer ${GCC_FLAGS_STACK} -no-pie")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -DDEBUG")
    endif(CMAKE_COMPILER_IS_GNUCC)
    if(INSTRUMENTATION)
        if(CMAKE_COMPILER_IS_GNUCC)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_FLAGS_SANITIZE_ALLLEAK}")
        endif(CMAKE_COMPILER_IS_GNUCC)
    elseif(INSTRUMENTATION_UNDEFINED)
        if(CMAKE_COMPILER_IS_GNUCC)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_FLAGS_SANITIZE_UNDEFINED}")
        endif(CMAKE_COMPILER_IS_GNUCC)
    elseif(INSTRUMENTATION_THREAD)
        if(CMAKE_COMPILER_IS_GNUCC)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_FLAGS_SANITIZE_THREAD}")
        endif(CMAKE_COMPILER_IS_GNUCC)
    endif(INSTRUMENTATION)
elseif(GPROF)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -ggdb -pg")
elseif(PERF_ANALYSIS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -ggdb")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
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
        set(CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} -no-pie")
        set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
    endif(CMAKE_COMPILER_IS_GNUCC)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG")
endif(DEBUG)

if(DONT_USE_RTTI)
    message(STATUS "${PROJECT_NAME} RTTI disabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fno-rtti")
    #set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -fno-rtti")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fno-rtti")
else()
    message(STATUS "${PROJECT_NAME} RTTI enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -frtti")
    #set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -frtti")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -frtti")
endif(DONT_USE_RTTI)

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

if(${OS_NAME} STREQUAL "linux")
    if(CMAKE_COMPILER_IS_GNUCC)
        set(CMAKE_CXX_STANDARD_LIBRARIES  "${CMAKE_CXX_STANDARD_LIBRARIES} -latomic")
    else()
        set(CMAKE_CXX_STANDARD_LIBRARIES  "${CMAKE_CXX_STANDARD_LIBRARIES} -latomic")
    endif(CMAKE_COMPILER_IS_GNUCC)
endif()

if (DEFINED EMSCRIPTEN)
    # See https://emscripten.org/docs/tools_reference/settings_reference.html
    #
    # set(EMS_FLAGS "--use-port=sdl2 --use-port=sdl2_image --use-port=sdl2_ttf")
    set(EMS_FLAGS "-s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -Wno-unused-command-line-argument")
    set(EMS_FLAGS "${EMS_FLAGS} -s WASM=1 -s LZ4=1")
    set(EMS_FLAGS "${EMS_FLAGS} -s FULL_ES2=1") # or even FULL_ES3 ?
    set(EMS_FLAGS "${EMS_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    # set(EMS_FLAGS "${EMS_FLAGS} -pthread") # fights w/ ALLOW_MEMORY_GROWTH
    # set(EMS_FLAGS "${EMS_FLAGS} -s MEMORY64=1") # wasm64 end-to-end: wasm32 object file can't be linked in wasm64 mode
    # set(EMS_FLAGS "${EMS_FLAGS} -s ASSERTIONS=1")
    set(EMS_FLAGS "${EMS_FLAGS} -s STACK_OVERFLOW_CHECK=1") # cheap cockie magic, enables CHECK_NULL_WRITES
    # set(EMS_EXE_FLAGS "-s SIDE_MODULE=1")
    set(EMS_EXE_FLAGS "")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMS_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMS_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMS_FLAGS}")
    set(USE_SFML OFF)
    # set(DONT_USE_RTTI ON)
    set(USE_LIBUNWIND OFF)
    set(SDL2_LIBS "")
else()
    set(SDL2_LIBS "SDL2;SDL2_image;SDL2_ttf")
endif()

set (LIB_INSTALL_DIR "lib${LIB_SUFFIX}" CACHE PATH "Installation path for libraries")

message(STATUS "${PROJECT_NAME} CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message(STATUS "${PROJECT_NAME} CMAKE_SHARED_LINKER_FLAGS = ${CMAKE_SHARED_LINKER_FLAGS}")
message(STATUS "${PROJECT_NAME} CMAKE_EXE_LINKER_FLAGS = ${CMAKE_EXE_LINKER_FLAGS}")
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_LIBCURL=1")
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_LIBUNWIND=1")
else()
    set(LIBUNWIND_LIBNAME "")
endif()
message(STATUS "${PROJECT_NAME} USE_LIBUNWIND ${USE_LIBUNWIND} -> libname ${LIBUNWIND_LIBNAME}")

string(TIMESTAMP BUILD_TSTAMP "%Y-%m-%d %H:%M:%S")

endmacro()

