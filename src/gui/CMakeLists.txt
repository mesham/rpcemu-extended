include(${CMAKE_SOURCE_DIR}/cmake/FindWxWidgets.cmake)

set(GUI_SOURCES
    main.cpp
    data_paths.cpp
    main_frame.cpp
    main_frame_menus.cpp
    headless_bridge.cpp
    headless_main.cpp
    emulator_panel.cpp
    emulator_host.cpp
    emulator_snapshot.cpp
    gui_preferences.cpp
    input_helpers.cpp
    config_selector_dialog.cpp
    config_paths.cpp
    machine_edit_dialog.cpp
    machine_inspector_window.cpp
    about_dialog.cpp
    nat_edit_dialog.cpp
    nat_list_dialog.cpp
    serial_dialog.cpp
    parallel_dialog.cpp
    settings.cpp
    plt_sound.cpp
    toolbar_icons.cpp
    keyboard_x.c
)

if(RPCEMU_ENABLE_VNC)
    list(APPEND GUI_SOURCES vnc_server.cpp vnc_dialog.cpp)
endif()

if(RPCEMU_DYNAREC)
    set(RPCEMU_GUI_TARGET rpcemu-recompiler)
else()
    set(RPCEMU_GUI_TARGET rpcemu-interpreter)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RPCEMU_GUI_TARGET "${RPCEMU_GUI_TARGET}-debug")
endif()

add_executable(${RPCEMU_GUI_TARGET} ${GUI_SOURCES})

target_include_directories(${RPCEMU_GUI_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_definitions(${RPCEMU_GUI_TARGET} PRIVATE
    RPCEMU_VERSION="${PROJECT_VERSION}"
    RPCEMU_INSTALL_DATADIR="${CMAKE_INSTALL_PREFIX}/share/rpcemu"
)

if(RPCEMU_DYNAREC)
    target_compile_definitions(${RPCEMU_GUI_TARGET} PRIVATE RPCEMU_BUILD_DYNAREC=1)
endif()

if(RPCEMU_ENABLE_VNC)
    target_compile_definitions(${RPCEMU_GUI_TARGET} PRIVATE RPCEMU_VNC)
endif()

target_link_libraries(${RPCEMU_GUI_TARGET} PRIVATE
    rpcemu_core
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(SDL2 REQUIRED sdl2)
target_include_directories(${RPCEMU_GUI_TARGET} PRIVATE ${GTK3_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
target_link_libraries(${RPCEMU_GUI_TARGET} PRIVATE ${GTK3_LIBRARIES} ${SDL2_LIBRARIES})

if(RPCEMU_ENABLE_VNC)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBVNCSERVER REQUIRED libvncserver)
    target_include_directories(${RPCEMU_GUI_TARGET} PRIVATE ${LIBVNCSERVER_INCLUDE_DIRS})
    target_link_libraries(${RPCEMU_GUI_TARGET} PRIVATE ${LIBVNCSERVER_LIBRARIES})
endif()

rpcemu_setup_wxwidgets(${RPCEMU_GUI_TARGET})

set_target_properties(${RPCEMU_GUI_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS ${RPCEMU_GUI_TARGET} RUNTIME DESTINATION bin)
