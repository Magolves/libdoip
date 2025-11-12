
add_executable(${DOIP_NAME}_tests
    ByteArray_Test.cpp
    DoIPMessage_Test.cpp
    DoIPServer_Test.cpp
    Main_Test.cpp
    MacAddress_Test.cpp
    Identifiers_Test.cpp
    VehicleIdentification_Test.cpp
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

