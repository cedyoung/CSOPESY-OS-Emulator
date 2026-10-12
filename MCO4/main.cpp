// CSOPESY Desktop OS Emulator
//
// Layer order, bottom to top:
//   1. Desktop - wallpaper, big clock + date, PWR button
//   2. App windows  - File Explorer, Settings, Task Manager
//   3. Taskbar - app launcher buttons, always kept on top

#include <iostream>
#include <cfloat>
#include <cstdio>
#include <ctime>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "task_manager.h"

static const float TASKBAR_HEIGHT = 50.0f;

// Displays GLFW errors in the terminal
static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": "
              << description << std::endl;
}


// DESKTOP
static void DrawWallpaper(ImDrawList* dl, ImVec2 pos, ImVec2 size)
{
    const ImVec2 end(pos.x + size.x, pos.y + size.y);

    const ImU32 top = IM_COL32(255, 120, 80, 255);
    const ImU32 bottom = IM_COL32(88, 40, 120, 255);
    dl->AddRectFilledMultiColor(pos, end, top, top, bottom, bottom);
}

static void DrawClock(ImDrawList* dl, ImVec2 pos, ImVec2 size)
{
    std::time_t now = std::time(nullptr);
    std::tm t{};
#ifdef _WIN32
    localtime_s(&t, &now);
#else
    localtime_r(&now, &t);
#endif
    char timeStr[16], dateStr[64];
    std::strftime(timeStr, sizeof timeStr, "%I:%M %p", &t);
    std::strftime(dateStr, sizeof dateStr, "%A, %B %d, %Y", &t);

    const float margin = 20.0f;
    const float bigSize = 100.0f;
    const float dateSize = 50.0f;
    ImFont* font = ImGui::GetFont();

    const ImVec2 timeSz = font->CalcTextSizeA(bigSize, FLT_MAX, 0.0f, timeStr);
    const ImVec2 dateSz = font->CalcTextSizeA(dateSize, FLT_MAX, 0.0f, dateStr);

    const ImVec2 datePos(pos.x + size.x - dateSz.x - margin, pos.y + margin);
    const ImVec2 timePos(pos.x + size.x - timeSz.x - margin, datePos.y + dateSz.y + 2.0f);

    dl->AddText(font, bigSize, ImVec2(timePos.x + 2, timePos.y + 2), IM_COL32(0, 0, 0, 140), timeStr);
    dl->AddText(font, bigSize, timePos, IM_COL32(255, 255, 255, 255), timeStr);
    dl->AddText(font, dateSize, ImVec2(datePos.x + 1, datePos.y + 1), IM_COL32(0, 0, 0, 140), dateStr);
    dl->AddText(font, dateSize, datePos, IM_COL32(220, 220, 235, 230), dateStr);
}

static bool DrawPowerButton()
{
    ImGui::SetCursorPos(ImVec2(20.0f, 20.0f));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.15f, 0.15f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.20f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.10f, 0.10f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

    const bool clicked = ImGui::Button("PWR", ImVec2(72.0f, 36.0f));

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Shut down");
    return clicked;
}

// Returns true when the PWR button was clicked.
static bool DrawDesktop()
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus | // keeps the desktop at the very bottom
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::Begin("##Desktop", nullptr, flags);
    ImGui::PopStyleVar(3);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 pos  = vp->Pos;
    const ImVec2 size = vp->Size;

    DrawWallpaper(dl, pos, size);
    DrawClock(dl, pos, size);
    const bool shutdown = DrawPowerButton();

    ImGui::End();
    return shutdown;
}


// APPS

// File Explorer
static void RenderFileExplorer(bool* open) {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.15f, 70.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(560, 360), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("File Explorer", open)) {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("Path");
    ImGui::SameLine();
    ImGui::Text("This PC > CSOPESY OS (C:) > User > Me");
    ImGui::Separator();

    static int selectedFolder = -1;
    static int selectedFile = -1;

    const char* folders[] = { "Document", "Downloads", "Pictures", "Videos" };
    const char* files[] = { "Final.txt", "FinalFinal.txt", "ReallyFinal.txt" };

    if (ImGui::BeginTable("ExplorerTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Folders");
        ImGui::TableSetupColumn("Files");
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        for (int i = 0; i < IM_ARRAYSIZE(folders); i++) {
            std::string label = std::string("[DIR] ") + folders[i];
            if (ImGui::Selectable(label.c_str(), selectedFolder == i)) {
                selectedFolder = i;
            }
        }

        ImGui::TableSetColumnIndex(1);
        for (int i = 0; i < IM_ARRAYSIZE(files); i++) {
            std::string label = std::string("[FILES] ") + files[i];
            if (ImGui::Selectable(label.c_str(), selectedFile == i)) {
                selectedFile = i;
            }
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

// Settings
static void RenderSettings(bool* open) {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.30f, 110.0f),
        ImGuiCond_FirstUseEver
    );
    ImGui::SetNextWindowSize(ImVec2(400, 240), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Settings", open)) {
        ImGui::End();
        return;
    }

    ImGui::Text("System Information");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Operating System:\t\tCSOPESY OS");
    ImGui::Text("Version:\t\t\t\t1.0.0");
    ImGui::Text("Architecture:\t\t\tx64");

    ImGui::Text("Display:\t\t\t\t%.0f x %.0f", io.DisplaySize.x, io.DisplaySize.y);
    ImGui::Text("Theme:\t\t\t\t\tDark");

    ImGui::End();
}

// TASKBAR  (app launchers only; the clock and PWR now live on the desktop)

static void RenderTaskbar(bool* showFiles, bool* showSettings, bool* showTaskManager) {
    ImGuiIO& io = ImGui::GetIO();
    const float width = io.DisplaySize.x;
    const float height = TASKBAR_HEIGHT;

    ImGui::SetNextWindowPos(ImVec2(0.0f, io.DisplaySize.y - height));
    ImGui::SetNextWindowSize(ImVec2(width, height));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.07f, 0.09f, 0.97f));

    ImGui::Begin("##Taskbar", nullptr, flags);

    const float btnH = TASKBAR_HEIGHT - 16.0f;

    auto appButton = [&](const char* label, float w, bool isOpen) -> bool {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, isOpen ? ImVec4(0.25f, 0.35f, 0.60f, 1.0f) : ImVec4(0.16f, 0.16f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.40f, 0.68f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.28f, 0.52f, 1.0f));

        bool clicked = ImGui::Button(label, ImVec2(w, btnH));
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        return clicked;
    };

    auto openOrFocus = [](bool* visible, const char* windowTitle) {
        if (!*visible) {
            *visible = true;
        }
        ImGui::SetWindowFocus(windowTitle);
    };

    ImGui::SetCursorPos(ImVec2(8.0f, 8.0f));

    if (appButton("Files", 90.0f, *showFiles)) {
        openOrFocus(showFiles, "File Explorer");
    }
    ImGui::SameLine(0.0f, 6.0f);

    if (appButton("Settings", 90.0f, *showSettings)) {
        openOrFocus(showSettings, "Settings");
    }
    ImGui::SameLine(0.0f, 6.0f);

    if (appButton("Task Manager", 130.0f, *showTaskManager)) {
        openOrFocus(showTaskManager, "Task Manager");
    }

    ImGuiWindow* taskbarWindow = ImGui::GetCurrentWindow();

    ImGui::End();

    // Keep the taskbar above every app window.
    ImGui::BringWindowToDisplayFront(taskbarWindow);

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

// MAIN

int main() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    // OpenGL / GLSL configuration
#ifdef __APPLE__
    const char* glslVersion = "#version 150";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    const char* glslVersion = "#version 130";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(
        1280, 720, "CSOPESY Desktop OS Emulator", nullptr, nullptr
    );

    if (window == nullptr) {
        std::cout << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Veto the OS close request (X button, Alt+F4). Only PWR may exit.
    glfwSetWindowCloseCallback(window, [](GLFWwindow* w) {
        glfwSetWindowShouldClose(w, GLFW_FALSE);
    });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // don't save window layout to imgui.ini

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    bool showFileExplorer = false;
    bool showSettings = false;
    bool showTaskManager = false;

    // Main loop
    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Desktop (bottom layer)
        if (DrawDesktop())
            glfwSetWindowShouldClose(window, GLFW_TRUE);

		// App windows (middle layer)
        if (showFileExplorer) {
            RenderFileExplorer(&showFileExplorer);
        }

        if (showSettings) {
            RenderSettings(&showSettings);
        }

        if (showTaskManager) {
            drawTaskManager(&showTaskManager);
        }

        // Taskbar (top layer)
        RenderTaskbar(&showFileExplorer, &showSettings, &showTaskManager);

        // Render
        ImGui::Render();

        int displayWidth, displayHeight;
        glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
