# libdoipserver CMakeLists.txt

# Create the doipserver library
add_library(doipserver
    src/DoIPConnection.cpp
    src/DoIPServer.cpp
    src/RoutingActivationHandler.cpp
    src/VehicleIdentificationHandler.cpp
)

# Set target properties
set_target_properties(doipserver PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(doipserver
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libdoip>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(doipserver
    PUBLIC
        doipcommon
    PRIVATE
        Threads::Threads
)

# Add alias for easier usage
add_library(libdoip::doipserver ALIAS doipserver)

# Tests
if(WITH_UNIT_TEST)
    # Routing Activation Handler Tests
    add_executable(routing_activation_tests
        test/RoutingActivationHandler_Test.cpp
    )

    target_link_libraries(routing_activation_tests
        PRIVATE
            doipserver
            doipcommon
            doctest::doctest
    )

    # Vehicle Identification Handler Tests
    add_executable(vehicle_identification_tests
        test/VehicleIdentificationHandler_Test.cpp
    )

    target_link_libraries(vehicle_identification_tests
        PRIVATE
            doipserver
            doipcommon
            doctest::doctest
    )

    # Use automatic test discovery for both test executables
    doctest_discover_tests(routing_activation_tests
        TEST_PREFIX "DoIPServer::RoutingActivation::"
        PROPERTIES
            LABELS "doipserver;routing"
            TIMEOUT 30
    )

    doctest_discover_tests(vehicle_identification_tests
        TEST_PREFIX "DoIPServer::VehicleIdentification::"
        PROPERTIES
            LABELS "doipserver;vehicle_identification"
            TIMEOUT 30
    )
endif()