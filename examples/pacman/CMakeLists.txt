
include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/examples/pacman
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(pacman game.cpp ghost.cpp maze.cpp pacman.cpp utils.cpp)
target_compile_options(pacman PRIVATE ${gfxbox2_CXX_FLAGS})
if (DEFINED EMSCRIPTEN)
    configure_file(pacman.html pacman.html COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/resources/fonts/freefont/FreeSansBold.ttf resources/fonts/freefont/FreeSansBold.ttf COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/resources/pacman/playfield_pacman.txt resources/pacman/playfield_pacman.txt COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/resources/pacman/playfield_pacman.png resources/pacman/playfield_pacman.png COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/resources/pacman/tiles_all.png resources/pacman/tiles_all.png COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/resources/pacman/intro.ogg resources/pacman/intro.ogg COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/resources/pacman/munch.wav resources/pacman/munch.wav COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/resources/pacman/eatfruit.ogg resources/pacman/eatfruit.ogg COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/resources/pacman/eatghost.ogg resources/pacman/eatghost.ogg COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/resources/pacman/death.ogg resources/pacman/death.ogg COPYONLY)
    set( TARGET_FILES "${CMAKE_CURRENT_BINARY_DIR}/pacman.wasm;pacman.html" )
    install(FILES ${TARGET_FILES} DESTINATION ${CMAKE_INSTALL_BINDIR})
    target_link_options(pacman PUBLIC ${gfxbox2_EXE_LINKER_FLAGS} "--embed-file=resources")
else()
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/resources/pacman DESTINATION ${CMAKE_INSTALL_DATADIR}/gfxbox2)
    target_link_options(pacman PUBLIC ${gfxbox2_EXE_LINKER_FLAGS})
endif()
target_link_libraries(pacman gfxbox2 ${SDL2_LIBS} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS pacman RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

