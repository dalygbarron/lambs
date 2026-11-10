cmake_minimum_required(VERSION 4.0)
project(lambs VERSION 0.1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_STANDARD 23)
add_executable(${PROJECT_NAME}
    src/def.c
    src/main.c
    src/utils.c
    src/ast.c
    src/ast_parse.c
    src/cJSON.c)
