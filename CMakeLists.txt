cmake_minimum_required(VERSION 3.15)
project(libdoip
    VERSION 1.0.0
    DESCRIPTION "DoIP (Diagnostic over Internet Protocol) Library"
    LANGUAGES CXX
)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(MAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(WITH_UNIT_TEST "Build unit tests" OFF)
option(WITH_EXAMPLES "Build examples" ON)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers in Debug builds" OFF)
option(ENABLE_STATIC_ANALYSIS "Enable static analysis tools" OFF)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(cmake/projectSettings.cmake)

# Find required packages
find_package(Threads REQUIRED)

# Find or fetch spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.12.0
    )
    # Install spdlog with the library
    set(SPDLOG_INSTALL ON CACHE BOOL "Generate the install target" FORCE)
    # Disable warnings as errors for spdlog to prevent compilation issues
    set(SPDLOG_BUILD_WARNINGS OFF CACHE BOOL "Enable warnings for spdlog" FORCE)
    FetchContent_MakeAvailable(spdlog)

    # Remove warnings as errors from spdlog target if it exists
    if(TARGET spdlog)
        get_target_property(spdlog_options spdlog COMPILE_OPTIONS)
        if(spdlog_options)
            list(REMOVE_ITEM spdlog_options "-Werror")
            set_target_properties(spdlog PROPERTIES COMPILE_OPTIONS "${spdlog_options}")
        endif()
        target_compile_options(spdlog PRIVATE -Wno-error)
    endif()
else()
    message(STATUS "spdlog found")
endif()

set(DOIP_NAME iso13400 CACHE STRING "DoIP library name")
set(SOURCES
    src/AliveCheckTimer.cpp
    src/DoIPClient.cpp
    src/DoIPConnection.cpp
    src/DoIPServer.cpp
    src/DoIPServerStateMachine.cpp
    src/Logger.cpp
    src/MacAddress.cpp
)


# Create the library
add_library(${DOIP_NAME}
    ${SOURCES}
)

# Set target properties
set_target_properties(${DOIP_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(${DOIP_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/lib${DOIP_NAME}>
)

# Link dependencies
target_link_libraries(${DOIP_NAME}
    PUBLIC
        spdlog::spdlog
    PRIVATE
        Threads::Threads
)

# Disable switch-default warning for this target since spdlog headers trigger it
target_compile_options(${DOIP_NAME} PRIVATE -Wno-switch-default)

# Install configuration
include(GNUInstallDirs)

# Add examples subdirectory
if (WITH_EXAMPLES)
    add_subdirectory(examples)
endif()

if (WITH_UNIT_TEST)
    # Enable testing
    enable_testing()

    # Find or download doctest
    find_package(doctest QUIET)
    if(NOT doctest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            doctest
            GIT_REPOSITORY https://github.com/doctest/doctest.git
            GIT_TAG        v2.4.11
        )
        FetchContent_MakeAvailable(doctest)
    endif()

    # Include doctest CMake module for test discovery
    include(cmake/doctest.cmake)
    add_subdirectory(test)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Install libraries
install(TARGETS ${DOIP_NAME}
    EXPORT lib${DOIP_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers
install(DIRECTORY inc/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/{DOIP_NAME})

# Create and install package config files
install(EXPORT lib${DOIP_NAME}Targets
    FILE lib${DOIP_NAME}Targets.cmake
    NAMESPACE lib${DOIP_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lib${DOIP_NAME}
)

# Generate package config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/lib${DOIP_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lib${DOIP_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/lib${DOIP_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lib${DOIP_NAME}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/lib${DOIP_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/lib${DOIP_NAME}ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lib${DOIP_NAME}
)
