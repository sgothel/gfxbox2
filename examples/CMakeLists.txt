include_directories(
  ${PROJECT_SOURCE_DIR}/include
)

# These examples use the standard separate compilation
if (DEFINED EMSCRIPTEN)
    set(SOURCES_IDIOMATIC_TARGETS "spacewars.cpp;spaceinv01.cpp;canonball.cpp;piviz.cpp;freefall01.cpp;pong01.cpp;tron.cpp;sandbox01.cpp;solarsystem.cpp;example01.cpp")
else()
    file(GLOB SOURCES_IDIOMATIC_TARGETS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
#    set(SOURCES_IDIOMATIC_TARGETS "spacewars.cpp;spaceinv01.cpp;canonball.cpp;solarsystem.cpp")
endif()

string( REPLACE ".cpp" "" BASENAMES_IDIOMATIC_TARGETS "${SOURCES_IDIOMATIC_TARGETS}" )

set( SDL_TARGETS_IDIOMATIC_TARGETS "" )
set( SDL_TARGETS_IDIOMATIC_FILES "" )
foreach( name ${BASENAMES_IDIOMATIC_TARGETS} )
    set(target ${name})
    set( SDL_TARGETS_IDIOMATIC_TARGETS "${SDL_TARGETS_IDIOMATIC_TARGETS};${target}" )
    add_executable(${target} ${name}.cpp)
    target_compile_options(${target} PUBLIC ${gfxbox2_CXX_FLAGS})
    if (DEFINED EMSCRIPTEN)
        configure_file(${name}.html ${name}.html COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/resources/fonts/freefont/FreeSansBold.ttf resources/fonts/freefont/FreeSansBold.ttf COPYONLY)
        configure_file(${PROJECT_SOURCE_DIR}/resources/spaceinv/spaceinv-sprites.png resources/spaceinv/spaceinv-sprites.png COPYONLY)
        set( SDL_TARGETS_IDIOMATIC_FILES "${SDL_TARGETS_IDIOMATIC_FILES};${CMAKE_CURRENT_BINARY_DIR}/${name}.wasm;${name}.html" )
        target_link_options(${target} PUBLIC ${gfxbox2_EXE_LINKER_FLAGS} "--embed-file=resources")
    else()
        target_link_options(${target} PUBLIC ${gfxbox2_EXE_LINKER_FLAGS})
    endif()
    target_link_libraries(${target} gfxbox2 ${SDL2_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endforeach()

set( SFML_TARGETS_IDIOMATIC_TARGETS "" )
if(USE_SFML)
    foreach( name ${BASENAMES_IDIOMATIC_TARGETS} )
        set( SFML_TARGETS_IDIOMATIC_TARGETS "${SFML_TARGETS_IDIOMATIC_TARGETS};${name}_sfml" )
        set(target ${name}_sfml)
        add_executable(${target} ${name}.cpp)
        target_compile_options(${target} PUBLIC ${gfxbox2_CXX_FLAGS})
        target_link_options(${target} PUBLIC ${gfxbox2_sfml_EXE_LINKER_FLAGS})
        target_link_libraries(${target} gfxbox2_sfml sfml-graphics sfml-window sfml-system ${CMAKE_THREAD_LIBS_INIT})
    endforeach()
endif(USE_SFML)

install(TARGETS ${SDL_TARGETS_IDIOMATIC_TARGETS} ${SFML_TARGETS_IDIOMATIC_TARGETS} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${SDL_TARGETS_IDIOMATIC_FILES} ${SFML_TARGETS_IDIOMATIC_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory (funcdraw)
