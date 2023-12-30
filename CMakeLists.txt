cmake_minimum_required(VERSION 3.27)
project(chaos VERSION 0.1 LANGUAGES CXX C)
file(
        GLOB imguiCppSources
        dependencies/imgui-1.90/*.cpp
        dependencies/imgui-1.90/backends/imgui_impl_opengl3.cpp
        dependencies/imgui-1.90/misc/cpp/imgui_stdlib.cpp
        dependencies/imgui-1.90/backends/imgui_impl_glfw.cpp
)
file(
        GLOB imguiHeaders
        dependencies/imgui-1.90/*.h
        dependencies/imgui-1.90/backends/imgui_impl_opengl3.h
        dependencies/imgui-1.90/misc/cpp/imgui_stdlib.h
        dependencies/imgui-1.90/backends/imgui_impl_glfw.h
)

find_package(OpenGL REQUIRED)
add_executable(chaos_linux
        src/main.cpp
        ${imguiCppSources}
        ${imguiHeaders}


)

add_subdirectory(dependencies/glfw-3.3.9)
target_link_libraries(chaos_linux ${OPENGL_LIBRARY} glfw)
target_include_directories(chaos_linux PUBLIC
        ${CMAKE_SOURCE_DIR}/dependencies/glfw-3.3.9/include
        ${CMAKE_SOURCE_DIR}/dependencies/imgui-1.90
        ${CMAKE_SOURCE_DIR}/dependencies/imgui-1.90/backends
)