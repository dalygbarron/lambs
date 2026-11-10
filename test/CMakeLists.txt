


add_executable(${PROJECT_NAME}-test
    utils.c)
target_link_libraries(${PROJECT_NAME}-test munit)
add_test(NAME tests COMMAND ${PROJECT_NAME}-test)