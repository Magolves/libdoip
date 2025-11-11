# Examples CMakeLists.txt

# DoIP Server Example
add_executable(exampleDoIPServer exampleDoIPServer.cpp)
target_link_libraries(exampleDoIPServer
    PRIVATE
    ${DOIP_NAME}
    Threads::Threads
)

# Set properties for the example
set_target_properties(exampleDoIPServer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# DoIP Client Example
add_executable(exampleDoIPClient exampleDoIPClient.cpp)
target_link_libraries(exampleDoIPClient
    PRIVATE
    ${DOIP_NAME}
)


# Set properties for the example
set_target_properties(exampleDoIPClient PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Disable switch-default warning for examples using spdlog
target_compile_options(exampleDoIPServer PRIVATE -Wno-switch-default)
target_compile_options(exampleDoIPClient PRIVATE -Wno-switch-default)

# Install examples (optional)
install(TARGETS exampleDoIPServer exampleDoIPClient
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
)