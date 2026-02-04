include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
    GIT_SHALLOW TRUE
)

# 关掉GLFW的docs tests examples install
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)

# imgui docking
# <name>_SOURCE_DIR
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glfw glm imgui)

if(NOT TARGET imgui_full)
    message(STATUS "imgui with GLFW and opengl3 backend")
    add_library(imgui_full STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        # 后端实现
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp    
    )
    # 依赖, header对外部可见
    target_include_directories(imgui_full PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    # 依赖glad
    # imgui_impl_opengl3.cpp 里会 include <glad/glad.h>
    target_include_directories(imgui_full PRIVATE
        "${CMAKE_SOURCE_DIR}/thirdparty/glad/include"
    )
    # 链接glfw
    # 后端需要用
    target_link_libraries(imgui_full PUBLIC glfw)
    message(STATUS "imgui setup complete")
endif()

message(STATUS "ImGui source dir: ${imgui_SOURCE_DIR}")
