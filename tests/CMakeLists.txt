# Unit tests. The JIT flag tester needs the amd64 recompiler and its test hooks
# (compiled into rpcemu_core when RPCEMU_JIT_TEST is set, see src/CMakeLists.txt).
if(RPCEMU_DYNAREC AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    add_executable(test_jit_flags test_jit_flags.c test_stubs.c)
    target_link_libraries(test_jit_flags PRIVATE rpcemu_core m)
    add_test(NAME jit_flags COMMAND test_jit_flags)
else()
    message(STATUS "Skipping JIT flag tests (requires amd64 dynarec build)")
endif()
