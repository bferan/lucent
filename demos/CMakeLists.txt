# Basic demo
add_executable(lucent-demo-basic basic/Basic.cpp)
target_link_libraries(lucent-demo-basic PRIVATE lucent)

# Copy shaders directory to the build output directory after build
add_custom_command(
    TARGET lucent-demo-basic POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/src/shaders
            $<TARGET_FILE_DIR:lucent-demo-basic>/shaders
    COMMENT "Copying shaders directory to build output"
)

# Copy data directory to the build output directory after build
add_custom_command(
    TARGET lucent-demo-basic POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/demos/basic/data
            $<TARGET_FILE_DIR:lucent-demo-basic>/data
    COMMENT "Copying data directory to build output"
)