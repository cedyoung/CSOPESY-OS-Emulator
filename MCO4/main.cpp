#include <iostream>
#include <ctime>
#include <string>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "task_manager.h"

static const float TASKBAR_HEIGHT = 50.0f;

// Displays GLFW errors in the terminal for easier debugging.
static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": "
              << description << std::endl;
}

// File Exp
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

    const char* folders[] = { "Document", "Downloads", "Pictures", "Vidoes" };
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

    ImGui::Text("Operating System:		CSOPESY OS");
    ImGui::Text("Version:				1.0.0");
    ImGui::Text("Architecture:			x64");

    ImGui::Text("Display:				%.0f x %.0f", io.DisplaySize.x, io.DisplaySize.y);
    ImGui::Text("Theme:					Dark");

    ImGui::End();

}

// Taskbar
static void RenderTaskbar(GLFWwindow* window, bool* showFiles, bool* showSettings, bool* showTaskManager) {
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

    // L
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

    // R
    std::time_t now = std::time(nullptr);
    std::tm localTm{};

#if defined(_WIN32)
    localtime_s(&localTm, &now);
#else
    localtime_r(&now, &localTm);
#endif

    char clockText[32];
    std::strftime(clockText, sizeof(clockText), "%I:%M:%S %p", &localTm);

    const float pwrW = 64.0f;
    const float gap = 14.0f;
    const float clockW = ImGui::CalcTextSize("00:00:00 PM").x;

    float pwrX = width - 8.0f - pwrW;
    float clockX = pwrX - gap - clockW;

    ImGui::SetCursorPos(ImVec2(clockX, (height - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::TextUnformatted(clockText);

    ImGui::SetCursorPos(ImVec2(pwrX, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.20f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.10f, 0.10f, 1.0f));


    if (ImGui::Button("PWR", ImVec2(pwrW, btnH))) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    ImGuiWindow* taskbarWindow = ImGui::GetCurrentWindow();

    ImGui::End();

    ImGui::BringWindowToDisplayFront(taskbarWindow);

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}



int main() {
    // Set GLFW error callback before initialization
    glfwSetErrorCallback(glfwErrorCallback);

    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW." << std::endl;
        return -1;
    }

    // OpenGL / GLSL configuration start
    #ifdef __APPLE__

        const char* glslVersion = "#version 150";

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(
            GLFW_OPENGL_PROFILE,
            GLFW_OPENGL_CORE_PROFILE
        );
        glfwWindowHint(
            GLFW_OPENGL_FORWARD_COMPAT,
            GL_TRUE
        );

    #else

        // Windows OpenGL settings
        const char* glslVersion = "#version 130";

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    #endif
    // OpenGL / GLSL configuration end

    // Create application window
    GLFWwindow* window = glfwCreateWindow(
        1280,
        720,
        "CSOPESY Desktop OS Emulator",
        nullptr,
        nullptr
    );

    if (window == nullptr) {
        std::cout << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Enable VSync.
    glfwSwapInterval(1);


    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);


    // Controls whether Task Manager is currently visible
    bool showTaskManager = false;

    bool showFileExplorer = false;
    bool showSettings = false;


    // Main application loop
    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (showFileExplorer) {
        RenderFileExplorer(&showFileExplorer);
        }

        if (showSettings) {
        RenderSettings(&showSettings);
        }
        
        // TASK MANAGER INTEGRATION
        if (showTaskManager) {
            drawTaskManager(&showTaskManager);
        }

        RenderTaskbar(
            window,
            &showFileExplorer,
            &showSettings,
            &showTaskManager
        );
        
        // Render
        ImGui::Render();

        int displayWidth;
        int displayHeight;

        glfwGetFramebufferSize(
            window,
            &displayWidth,
            &displayHeight
        );

        glViewport(
            0,
            0,
            displayWidth,
            displayHeight
        );

        glClearColor(
            0.10f,
            0.10f,
            0.10f,
            1.00f
        );

        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(
            ImGui::GetDrawData()
        );

        glfwSwapBuffers(window);
    }


    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
