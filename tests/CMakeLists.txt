add_executable(test-uvtls main.c test.c server.c test-ring-buf.c test-client.c)
add_dependencies(test-uvtls uvtls)
target_include_directories(test-uvtls PRIVATE ../src)
target_include_directories(test-uvtls PRIVATE ${INCLUDE_DIRS})
target_link_libraries(test-uvtls uvtls ${LIBRARIES})
