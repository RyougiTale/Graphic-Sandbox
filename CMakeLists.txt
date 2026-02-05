cmake_minimum_required(VERSION 3.25)

# beta版本
set(PROJECT_VERSION 0.0.1)
project(GraphicSandbox VERSION ${PROJECT_VERSION} LANGUAGES CXX C)
message(STATUS "building ${PROJECT_NAME} ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# compile_commands.json生成 给IDE使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 可执行文件
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# 静态库 lib(windows导入符号) / a
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# 动态库 dll(windows) / so
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(cmake/FetchDeps.cmake)

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/Glad.cmake")
    file(COPY "${CMAKE_SOURCE_DIR}/cmake/Glad.cmake" 
         DESTINATION "${CMAKE_SOURCE_DIR}/thirdparty/glad/")

    # 重命名
    file(RENAME "${CMAKE_SOURCE_DIR}/thirdparty/glad/Glad.cmake" 
                "${CMAKE_SOURCE_DIR}/thirdparty/glad/CMakeLists.txt")
endif()

add_subdirectory(thirdparty/glad)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h" "include/*.h")

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/src 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    glad 
    imgui_full
    glfw 
    glm::glm
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
