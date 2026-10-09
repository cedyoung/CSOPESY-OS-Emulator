#!/bin/bash

echo "Building MCO4..."

xcrun --sdk macosx clang++ -std=c++17 \
-DGL_SILENCE_DEPRECATION \
-isysroot "$(xcrun --sdk macosx --show-sdk-path)" \
-isystem "$(xcrun --sdk macosx --show-sdk-path)/usr/include/c++/v1" \
main.cpp \
task_manager.cpp \
imgui/imgui.cpp \
imgui/imgui_draw.cpp \
imgui/imgui_tables.cpp \
imgui/imgui_widgets.cpp \
imgui/backends/imgui_impl_glfw.cpp \
imgui/backends/imgui_impl_opengl3.cpp \
-Iimgui \
-Iimgui/backends \
-I"$(brew --prefix glfw)/include" \
-L"$(brew --prefix glfw)/lib" \
-lglfw \
-framework Cocoa \
-framework OpenGL \
-framework IOKit \
-framework CoreVideo \
-o MCO4

if [ $? -ne 0 ]; then
    echo "BUILD FAILED."
    exit 1
fi

echo "BUILD SUCCESSFUL."
echo "Starting MCO4..."

./MCO4
