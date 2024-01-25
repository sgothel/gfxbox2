include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

# These examples use the standard separate compilation
file(GLOB SOURCES_IDIOMATIC_EXAMPLES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
# file(GLOB SOURCES_IDIOMATIC_EXAMPLES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "piviz.cpp")

string( REPLACE ".cpp" "" BASENAMES_IDIOMATIC_EXAMPLES "${SOURCES_IDIOMATIC_EXAMPLES}" )

set( SDL_TARGETS_IDIOMATIC_EXAMPLES "" )
foreach( name ${BASENAMES_IDIOMATIC_EXAMPLES} )
    set( SDL_TARGETS_IDIOMATIC_EXAMPLES "${SDL_TARGETS_IDIOMATIC_EXAMPLES};${name}" )
    add_executable(${name} ${name}.cpp ../src/pixel.cpp ../src/sdl_subsys.cpp)
    target_link_libraries(${name} SDL2 SDL2_image SDL2_ttf ${CMAKE_THREAD_LIBS_INIT})
endforeach()

set( SFML_TARGETS_IDIOMATIC_EXAMPLES "" )
if(USE_SFML)
    foreach( name ${BASENAMES_IDIOMATIC_EXAMPLES} )
        set( SFML_TARGETS_IDIOMATIC_EXAMPLES "${SFML_TARGETS_IDIOMATIC_EXAMPLES};${name}_sfml" )
        add_executable(${name}_sfml ${name}.cpp ../src/pixel.cpp ../src/sfml_subsys.cpp)
        target_link_libraries(${name}_sfml sfml-graphics sfml-window sfml-system ${CMAKE_THREAD_LIBS_INIT})
    endforeach()
endif(USE_SFML)

install(TARGETS ${SDL_TARGETS_IDIOMATIC_EXAMPLES} ${SFML_TARGETS_IDIOMATIC_EXAMPLES} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory (funcdraw)
