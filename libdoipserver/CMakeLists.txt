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
    add_executable(doipserver_tests
        test/RoutingActivationHandler_Test.cpp
        test/VehicleIdentificationHandler_Test.cpp
    )

    target_link_libraries(doipserver_tests
        PRIVATE
            doipserver
            doipcommon
    )

    add_test(NAME DoIPServerTests COMMAND doipserver_tests)
endif()