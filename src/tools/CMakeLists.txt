# Standalone host-side client for the HostCmd channel. Does not link the
# emulator core - it is just a small socket client.

add_executable(rpcemu-run rpcemu_run.c)
set_target_properties(rpcemu-run PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
install(TARGETS rpcemu-run RUNTIME DESTINATION bin)

# rpcemu-shell is the same binary; its interactive mode is selected by argv[0].
# Ship it as a symlink so both names are available.
add_custom_command(TARGET rpcemu-run POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink rpcemu-run
            ${CMAKE_BINARY_DIR}/bin/rpcemu-shell
    COMMENT "Creating rpcemu-shell symlink")
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \
    rpcemu-run \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/rpcemu-shell)")
