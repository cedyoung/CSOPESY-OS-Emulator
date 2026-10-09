@echo off

echo Building MCO4...

g++ -std=c++17 ^
main.cpp ^
task_manager.cpp ^
imgui\imgui.cpp ^
imgui\imgui_draw.cpp ^
imgui\imgui_tables.cpp ^
imgui\imgui_widgets.cpp ^
imgui\backends\imgui_impl_glfw.cpp ^
imgui\backends\imgui_impl_opengl3.cpp ^
-Iimgui ^
-Iimgui\backends ^
-Iglfw\include ^
-Lglfw\lib-mingw-w64 ^
-lglfw3 ^
-lopengl32 ^
-lgdi32 ^
-luser32 ^
-lshell32 ^
-o MCO4.exe

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo BUILD FAILED.
    pause
    exit /b 1
)

echo.
echo BUILD SUCCESSFUL.
echo Starting MCO4...
echo.

MCO4.exe

pause
