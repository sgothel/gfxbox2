
find_package(FLEX REQUIRED)

find_package(BISON REQUIRED)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/examples/funcdraw
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

BISON_TARGET(InfixCalcParser infix_calc_parser.yy ${CMAKE_CURRENT_BINARY_DIR}/infix_calc_parser.cpp
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/infix_calc_parser.hpp)

message(STATUS "${PROJECT_NAME} flex found, FLEX_INCLUDE_DIRS ${FLEX_INCLUDE_DIRS}")
FLEX_TARGET(InfixCalcScanner1 infix_calc_scanner1.ll  ${CMAKE_CURRENT_BINARY_DIR}/infix_calc_scanner1.cpp
      COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/infix_calc_scanner1.hpp" )
ADD_FLEX_BISON_DEPENDENCY(InfixCalcScanner1 InfixCalcParser)

add_executable(funcdraw funcdraw.cpp infix_calc.cpp rpn_calc.cpp ${BISON_InfixCalcParser_OUTPUTS} ${FLEX_InfixCalcScanner1_OUTPUTS})
target_compile_options(funcdraw PRIVATE ${gfxbox2_CXX_FLAGS} "-DSCANNER_FLEX" "-Wno-error=unused-but-set-variable" "-Wno-unused-but-set-variable")
set( SDL_TARGETS_IDIOMATIC_FILES "" )
if (DEFINED EMSCRIPTEN)
    configure_file(funcdraw.html funcdraw.html COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/resources/fonts/freefont/FreeSansBold.ttf resources/fonts/freefont/FreeSansBold.ttf COPYONLY)
    set( SDL_TARGETS_IDIOMATIC_FILES "${SDL_TARGETS_IDIOMATIC_FILES};${CMAKE_CURRENT_BINARY_DIR}/funcdraw.wasm;funcdraw.html" )
    target_link_options(funcdraw PUBLIC ${gfxbox2_EXE_LINKER_FLAGS} "--embed-file=resources")
else()
    target_link_options(funcdraw PUBLIC ${gfxbox2_EXE_LINKER_FLAGS})
endif()
target_link_libraries(funcdraw gfxbox2 ${SDL2_LIBS} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS funcdraw RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${SDL_TARGETS_IDIOMATIC_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})


