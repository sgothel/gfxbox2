
find_package(FLEX REQUIRED)

find_package(BISON REQUIRED)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${FLEX_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

BISON_TARGET(InfixCalcParser infix_calc_parser.yy ${CMAKE_CURRENT_BINARY_DIR}/infix_calc_parser.cpp
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/infix_calc_parser.hpp)

message(STATUS "${PROJECT_NAME} flex found, FLEX_INCLUDE_DIRS ${FLEX_INCLUDE_DIRS}")
FLEX_TARGET(InfixCalcScanner1 infix_calc_scanner1.ll  ${CMAKE_CURRENT_BINARY_DIR}/infix_calc_scanner1.cpp
      COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/infix_calc_scanner1.hpp" )
ADD_FLEX_BISON_DEPENDENCY(InfixCalcScanner1 InfixCalcParser)
add_executable(funcdraw funcdraw.cpp ../../src/pixel.cpp ../../src/sdl_subsys.cpp infix_calc.cpp ${BISON_InfixCalcParser_OUTPUTS} ${FLEX_InfixCalcScanner1_OUTPUTS})
target_compile_options(funcdraw PRIVATE -DSCANNER_FLEX)
target_link_libraries(funcdraw SDL2 ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS funcdraw RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


