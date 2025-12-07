
add_executable(${DOIP_NAME}_tests
    ByteArray_Test.cpp
    DoIPDefaultConnection_Test.cpp
    DoIPMessage_Test.cpp
    DoIPServer_Test.cpp
    Identifiers_Test.cpp
    MacAddress_Test.cpp
    Main_Test.cpp
    ThreadSafeQueue_Test.cpp
    TimerManager_Test.cpp
    VehicleIdentification_Test.cpp
    uds/UdsMock_Test.cpp
)

target_link_libraries(${DOIP_NAME}_tests
    PRIVATE
        ${DOIP_NAME}
        doctest::doctest
)

# Configure relaxed clang-tidy settings for tests (disable magic-numbers and nodiscard warnings)
configure_clang_tidy_for_tests(${DOIP_NAME}_tests)

# Use automatic test discovery instead of manual add_test
doctest_discover_tests(${DOIP_NAME}_tests
    PROPERTIES
        LABELS "${DOIP_NAME}"
        TIMEOUT 30
)

# Integration fixture: start exampleDoIPServer as a daemon
add_test(NAME Integration_StartExampleServer
    COMMAND $<TARGET_FILE:exampleDoIPServer> --daemonize
)
set_tests_properties(Integration_StartExampleServer PROPERTIES FIXTURES_SETUP example_server TIMEOUT 10)

# Integration test: run discover against the running server
add_test(NAME Integration_DiscoverRuns
    COMMAND $<TARGET_FILE:exampleDoIPDiscover>
)
set_tests_properties(Integration_DiscoverRuns PROPERTIES FIXTURES_REQUIRED example_server TIMEOUT 10)

# Teardown fixture: stop exampleDoIPServer
add_test(NAME Integration_StopExampleServer
    COMMAND pkill -f exampleDoIPServer
)
set_tests_properties(Integration_StopExampleServer PROPERTIES FIXTURES_CLEANUP example_server TIMEOUT 10)

