cmake_minimum_required(VERSION 3.25)
project(glad LANGUAGES C)

add_library(glad STATIC src/glad.c)
target_include_directories(glad PUBLIC include)

# linux需要链接dl
if(UNIX AND NOT APPLE)
    target_link_libraries(glad PRIVATE dl)
endif()