cmake_minimum_required(VERSION 3.10)

project(VM)

add_library(lib STATIC gen.c)

target_include_directories(lib
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        )

add_executable(main main.c)

target_link_libraries(main PRIVATE lib)