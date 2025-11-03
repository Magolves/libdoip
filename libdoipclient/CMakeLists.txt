# libdoipclient CMakeLists.txt

# Create the doipclient library
add_library(doipclient
    src/DoIPClient.cpp
)

# Set target properties
set_target_properties(doipclient PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(doipclient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libdoip>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(doipclient
    PUBLIC
        doipcommon
    PRIVATE
        Threads::Threads
)

# Add alias for easier usage
add_library(libdoip::doipclient ALIAS doipclient)

# Tests
if(BUILD_TESTING)
    add_executable(doipclient_tests
        test/DoIPClient_Test.cpp
    )

    target_link_libraries(doipclient_tests
        PRIVATE
            doipclient
            doipcommon
    )

    add_test(NAME DoIPClientTests COMMAND doipclient_tests)
endif()