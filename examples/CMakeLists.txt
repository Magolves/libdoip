# Examples CMakeLists.txt

set (EXAMPLE_SOURCES
    exampleDoIPServer.cpp
    exampleDoIPClient.cpp
    exampleDoIPDiscover.cpp
)

foreach(example_source ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_source} NAME_WE)
    add_executable(${example_name} ${example_source})
    target_link_libraries(${example_name}
        PRIVATE
        ${DOIP_NAME}
        Threads::Threads
    )

    # Set properties for the example
    set_target_properties(${example_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    # Disable switch-default warning for examples using spdlog
    target_compile_options(${example_name} PRIVATE -Wno-switch-default)

    # Install examples (optional)
    install(TARGETS ${example_name}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
    )

endforeach()
