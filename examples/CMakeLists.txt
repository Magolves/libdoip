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

if (WITH_UNIT_TEST)
    enable_testing()
    # Integration fixture: start exampleDoIPServer as a daemon
    add_test(NAME Integration_StartExampleServer
        COMMAND $<TARGET_FILE:exampleDoIPServer> --daemonize --loopback
    )
    set_tests_properties(Integration_StartExampleServer PROPERTIES FIXTURES_SETUP example_server TIMEOUT 20)

    # Integration test: run discover against the running server
    add_test(NAME Integration_DiscoverRuns
       COMMAND $<TARGET_FILE:exampleDoIPDiscover> --loopback
    )
    set_tests_properties(Integration_DiscoverRuns PROPERTIES FIXTURES_REQUIRED example_server TIMEOUT 20)

    # Teardown fixture: stop exampleDoIPServer
    add_test(NAME Integration_StopExampleServer
       COMMAND pkill -f exampleDoIPServer
    )
    set_tests_properties(Integration_StopExampleServer PROPERTIES FIXTURES_CLEANUP example_server TIMEOUT 10)
endif()
