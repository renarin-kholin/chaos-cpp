cmake_minimum_required(VERSION 3.27)
project(chaos VERSION 0.1 LANGUAGES CXX C)
file(
        GLOB imguiCppSources
        dependencies/imgui/*.cpp
        dependencies/imgui/backends/imgui_impl_opengl3.cpp
        dependencies/imgui/misc/cpp/imgui_stdlib.cpp
        dependencies/imgui/backends/imgui_impl_glfw.cpp
)
file(
        GLOB imguiHeaders
        dependencies/imgui/*.h
        dependencies/imgui/backends/imgui_impl_opengl3.h
        dependencies/imgui/misc/cpp/imgui_stdlib.h
        dependencies/imgui/backends/imgui_impl_glfw.h
)

find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.4)
pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)

find_package(OpenGL REQUIRED)
add_executable(chaos_linux
        src/main.cpp
        ${imguiCppSources}
        ${imguiHeaders}
        src/Chaos.h
        src/Chaos.cpp
        src/Utils.h
        src/Utils.cpp


)

add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/datachannel)
target_link_libraries(chaos_linux ${OPENGL_LIBRARY} glfw datachannel nlohmann_json::nlohmann_json
        PkgConfig::gstreamer
        PkgConfig::gstreamer-sdp
        PkgConfig::gstreamer-app
        PkgConfig::gstreamer-video)
target_include_directories(chaos_linux PUBLIC
        ${CMAKE_SOURCE_DIR}/dependencies/glfw/include
        ${CMAKE_SOURCE_DIR}/dependencies/imgui
        ${CMAKE_SOURCE_DIR}/dependencies/imgui/backends
        ${CMAKE_SOURCE_DIR}/dependencies/datachannel/include
        ${CMAKE_SOURCE_DIR}/dependencies/json/include
)