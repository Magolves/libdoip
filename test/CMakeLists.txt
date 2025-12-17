
add_executable(${DOIP_NAME}_tests
    ByteArray_Test.cpp
    DoIPDefaultConnection_Test.cpp
    DoIPMessage_Test.cpp
    DoIPServer_Test.cpp
    Identifiers_Test.cpp
    MacAddress_Test.cpp
    Main_Test.cpp
    Misc_Test.cpp
    cli/ServerConfigCLI_Test.cpp
    Stream_Test.cpp
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

# ------------------------------------------------------------
# Optional: Test coverage task with HTML report (requires gcovr)
# ------------------------------------------------------------
find_program(GCOVR_PATH NAMES gcovr)
set(GCOVR_FOUND OFF)
if(GCOVR_PATH)
    set(GCOVR_FOUND ON)
    message(STATUS "gcovr found: ${GCOVR_PATH}")

    if(WITH_TEST_COV)
        message(STATUS "WITH_TEST_COV enabled")

        # Instrument tests and library for coverage
        target_compile_options(${DOIP_NAME}_tests PRIVATE --coverage)
        target_link_options(${DOIP_NAME}_tests PRIVATE --coverage)
        if(TARGET ${DOIP_NAME})
            target_compile_options(${DOIP_NAME} PRIVATE --coverage)
            target_link_options(${DOIP_NAME} PRIVATE --coverage)
        endif()

        # Create custom target to run tests and generate HTML coverage report
        add_custom_target(test_cov
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/test/reports
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
            COMMAND ${GCOVR_PATH} --root ${CMAKE_SOURCE_DIR} --filter ${CMAKE_SOURCE_DIR}/src --filter ${CMAKE_SOURCE_DIR}/inc --exclude ${CMAKE_BINARY_DIR} --exclude ${CMAKE_SOURCE_DIR}/test --html --html-details --output ${CMAKE_SOURCE_DIR}/test/reports/coverage.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Run tests with coverage and generate HTML report: test/reports/coverage.html"
        )
    else()
        message(STATUS "WITH_TEST_COV disabled")
    endif()
else()
    if(WITH_TEST_COV)
        message(WARNING "WITH_TEST_COV enabled but gcovr not found! Test coverage target will not be created.")
    else()
        message(STATUS "WITH_TEST_COV disabled")
    endif()
endif()