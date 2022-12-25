include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

# These examples use the standard separate compilation
file(GLOB SOURCES_IDIOMATIC_EXAMPLES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")

string( REPLACE ".cpp" "" BASENAMES_IDIOMATIC_EXAMPLES "${SOURCES_IDIOMATIC_EXAMPLES}" )

set( SDL_TARGETS_IDIOMATIC_EXAMPLES "" )
foreach( name ${BASENAMES_IDIOMATIC_EXAMPLES} )
    set( SDL_TARGETS_IDIOMATIC_EXAMPLES "${SDL_TARGETS_IDIOMATIC_EXAMPLES};${name}_sdl" )
    add_executable(${name}_sdl ${name}.cpp ../src/pixel.cpp ../src/sdl_subsys.cpp)
    target_link_libraries(${name}_sdl SDL2 SDL2_image SDL2_ttf)
endforeach()

set( SFML_TARGETS_IDIOMATIC_EXAMPLES "" )
if(USE_SFML)
    foreach( name ${BASENAMES_IDIOMATIC_EXAMPLES} )
        set( SFML_TARGETS_IDIOMATIC_EXAMPLES "${SFML_TARGETS_IDIOMATIC_EXAMPLES};${name}_sfml" )
        add_executable(${name}_sfml ${name}.cpp ../src/pixel.cpp ../src/sfml_subsys.cpp)
        target_link_libraries(${name}_sfml sfml-graphics sfml-window sfml-system)
    endforeach()
endif(USE_SFML)

install(TARGETS ${SDL_TARGETS_IDIOMATIC_EXAMPLES} ${SFML_TARGETS_IDIOMATIC_EXAMPLES} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory (funcdraw)
