# wxWidgets helper for Linux GTK builds.
function(rpcemu_setup_wxwidgets target)
    find_package(wxWidgets REQUIRED COMPONENTS core base)
    include(${wxWidgets_USE_FILE})
    target_link_libraries(${target} PRIVATE ${wxWidgets_LIBRARIES})
endfunction()
