
add_executable(${DOIP_NAME}_tests
    DoIPGenericHeaderHandler_Test.cpp
    DoIPMessage_Test.cpp
    Main_Test.cpp
    RoutingActivationHandler_Test.cpp
    VehicleIdentificationHandler_Test.cpp
)

target_link_libraries(${DOIP_NAME}_tests
    PRIVATE
        ${DOIP_NAME}
        doctest::doctest
)

# Use automatic test discovery instead of manual add_test
doctest_discover_tests(${DOIP_NAME}_tests
    PROPERTIES
        LABELS "${DOIP_NAME}"
        TIMEOUT 30
)

