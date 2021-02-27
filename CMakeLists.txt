cmake_minimum_required(VERSION 3.11.1 FATAL_ERROR)

project(WebAssembly-Experiments)

include_directories("${CMAKE_SOURCE_DIR}/inc"
                    "${CMAKE_SOURCE_DIR}/dependencies/inc")

set(project_headers
    inc/AnimatedMesh.h
    inc/GLTFLoader.h)

set(project_sources
    src/AnimatedMesh.cpp
    src/GLTFLoader.cpp
    src/Ten3DCubesWithCamera.cpp
    dependencies/src/cgltf/cgltf.c
	dependencies/src/imgui/imgui.cpp
	dependencies/src/imgui/imgui_demo.cpp
	dependencies/src/imgui/imgui_draw.cpp
	dependencies/src/imgui/imgui_impl_glfw.cpp
	dependencies/src/imgui/imgui_impl_opengl3.cpp
	dependencies/src/imgui/imgui_widgets.cpp
	dependencies/src/stb_image/stb_image.cpp)

# This path must be relative to the location of the build folder
set(project_resources "../resources@resources")

set(CMAKE_EXECUTABLE_SUFFIX ".html")

set(CMAKE_CXX_FLAGS "-O3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -o index.html --preload-file ${project_resources} --use-preload-plugins")

add_executable(${PROJECT_NAME} ${project_headers} ${project_sources})
