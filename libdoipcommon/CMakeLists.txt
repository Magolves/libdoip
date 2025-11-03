
# libdoipcommon CMakeLists.txt

# Create the doipcommon library
add_library(doipcommon
    src/AliveCheckTimer.cpp
    src/DiagnosticMessageHandler.cpp
    src/DoIPGenericHeaderHandler.cpp
)

# Set target properties
set_target_properties(doipcommon PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(doipcommon
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libdoip>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(doipcommon
    PRIVATE
        Threads::Threads
)

# Add alias for easier usage
add_library(libdoip::doipcommon ALIAS doipcommon)

# Tests
if(WITH_UNIT_TEST)
    add_executable(doipcommon_tests
        test/DoIPGenericHeaderHandler_Test.cpp
    )

    target_link_libraries(doipcommon_tests
        PRIVATE
            doipcommon
    )

    add_test(NAME DoIPCommonTests COMMAND doipcommon_tests)
endif()