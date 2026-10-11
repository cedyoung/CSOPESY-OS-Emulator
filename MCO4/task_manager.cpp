#include "task_manager.h"
#include "imgui.h"

// Dummy process information for MCO4.
// The project specification allows dummy CPU and memory values.
struct ProcessInfo {
    const char* name;
    float cpuUsage;
    int memoryUsage;
};


void drawTaskManager(bool* open) {

    // Default Task Manager window size
    ImGui::SetNextWindowSize(
        ImVec2(650, 400),
        ImGuiCond_FirstUseEver
    );

    // Do not draw anything if the window is closed
    if (!ImGui::Begin("Task Manager", open)) {
        ImGui::End();
        return;
    }


    // ----------------------------
    // TASK MANAGER HEADER
    // ----------------------------

    ImGui::Text("Task Manager");
    ImGui::Separator();

    ImGui::Text("Processes");
    ImGui::Spacing();


    // ----------------------------
    // DUMMY PROCESS DATA
    // ----------------------------

    ProcessInfo processes[] = {
        {"System",          1.2f, 120},
        {"CSOPESY.exe",     4.8f, 256},
        {"Explorer.exe",    2.1f, 310},
        {"Terminal.exe",    0.7f, 95},
        {"Chrome.exe",      6.4f, 420},
        {"Discord.exe",     3.2f, 280},
        {"Notepad.exe",     0.2f, 35}
    };


    // ----------------------------
    // PROCESS TABLE
    // ----------------------------

    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable(
        "ProcessTable",
        3,
        tableFlags
    )) {

        // Column headers
        ImGui::TableSetupColumn(
            "Process",
            ImGuiTableColumnFlags_WidthStretch
        );

        ImGui::TableSetupColumn(
            "CPU",
            ImGuiTableColumnFlags_WidthFixed,
            100.0f
        );

        ImGui::TableSetupColumn(
            "Memory",
            ImGuiTableColumnFlags_WidthFixed,
            120.0f
        );

        ImGui::TableHeadersRow();


        // Display each dummy process
        for (const ProcessInfo& process : processes) {

            ImGui::TableNextRow();


            // Process Name
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", process.name);


            // CPU Usage
            ImGui::TableSetColumnIndex(1);
            ImGui::Text(
                "%.1f%%",
                process.cpuUsage
            );


            // Memory Usage
            ImGui::TableSetColumnIndex(2);
            ImGui::Text(
                "%d MB",
                process.memoryUsage
            );
        }

        ImGui::EndTable();
    }


    ImGui::End();
}
