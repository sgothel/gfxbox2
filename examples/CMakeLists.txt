include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

# These examples use the standard separate compilation
if (DEFINED EMSCRIPTEN)
    set(SOURCES_IDIOMATIC_TARGETS "spacewars.cpp;piviz.cpp;freefall01.cpp")
else()
    file(GLOB SOURCES_IDIOMATIC_TARGETS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
endif()

string( REPLACE ".cpp" "" BASENAMES_IDIOMATIC_TARGETS "${SOURCES_IDIOMATIC_TARGETS}" )

set( SDL_TARGETS_IDIOMATIC_TARGETS "" )
set( SDL_TARGETS_IDIOMATIC_FILES "" )
foreach( name ${BASENAMES_IDIOMATIC_TARGETS} )
    set(target ${name})
    set( SDL_TARGETS_IDIOMATIC_TARGETS "${SDL_TARGETS_IDIOMATIC_TARGETS};${target}" )
    add_executable(${target} ${name}.cpp ../src/pixel.cpp ../src/sdl_subsys.cpp)
    target_link_libraries(${target} ${SDL2_LIBS} ${CMAKE_THREAD_LIBS_INIT})
    if (DEFINED EMSCRIPTEN)
        configure_file(${name}.html ${name}.html COPYONLY)
        configure_file(${CMAKE_SOURCE_DIR}/fonts/freefont/FreeSansBold.ttf fonts/freefont/FreeSansBold.ttf COPYONLY)
        set( SDL_TARGETS_IDIOMATIC_FILES "${SDL_TARGETS_IDIOMATIC_FILES};${CMAKE_CURRENT_BINARY_DIR}/${name}.wasm;${name}.html" )
        set_target_properties(${target} PROPERTIES LINK_FLAGS "--embed-file=fonts ${EMS_EXE_FLAGS}")
        set_target_properties(${target} PROPERTIES COMPILE_FLAGS "${EMS_EXE_FLAGS}")
    endif()
endforeach()

set( SFML_TARGETS_IDIOMATIC_TARGETS "" )
if(USE_SFML)
    foreach( name ${BASENAMES_IDIOMATIC_TARGETS} )
        set( SFML_TARGETS_IDIOMATIC_TARGETS "${SFML_TARGETS_IDIOMATIC_TARGETS};${name}_sfml" )
        add_executable(${name}_sfml ${name}.cpp ../src/pixel.cpp ../src/sfml_subsys.cpp)
        target_link_libraries(${name}_sfml sfml-graphics sfml-window sfml-system ${CMAKE_THREAD_LIBS_INIT})
    endforeach()
endif(USE_SFML)

install(TARGETS ${SDL_TARGETS_IDIOMATIC_TARGETS} ${SFML_TARGETS_IDIOMATIC_TARGETS} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${SDL_TARGETS_IDIOMATIC_FILES} ${SFML_TARGETS_IDIOMATIC_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})

if (NOT DEFINED EMSCRIPTEN)
    add_subdirectory (funcdraw)
endif()
