#include <algorithm>
#include <iostream>
#include <queue>

#include <glad/glad.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <nfd.h>

#include "common.h"

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

#if 1
void printPos(string name, float x, float y) {
    std::cout << name << " x: " << x << ", y:" << y << "\n";
}

void printPos(string name, ImVec2 pos) {
    std::cout << name << " x: " << pos.x << ", y:" << pos.y << "\n";
}

class CodeBlock {
    public:
        File* file;

        int import_level = -1;

        ImVec2 relative_pos = ImVec2(0, 0);
        ImVec2 size = ImVec2(120, 90);
        ImU32 color = IM_COL32(3, 83, 164, 255);

        void drawBlock(ImDrawList* draw_list, ImVec2& grid_pos, float zoom_level) {
            ImVec2 min_pos = ImVec2(relative_pos.x - (size.x / 2),
                                    relative_pos.y - (size.y / 2));
            ImVec2 max_pos = ImVec2(relative_pos.x + (size.x / 2),
                                    relative_pos.y + (size.y / 2));

            draw_list->AddRectFilled(min_pos, max_pos, color);
        }
};

struct {  // TODO: Move from global scope. Use static?
    vector<File*> files;  // Copy of files passed, to save temp changes.
    int selected_file_tab = 0;  // TODO: Change to pointer to file.

    int menu_width;
    int menu_height;

    vector<CodeBlock*> code_blocks;
} MenuState;

CodeBlock* getCodeBlockByFile(const File* file, vector<CodeBlock*> &code_blocks) {
    for (CodeBlock* code_block: code_blocks) {
        if (code_block->file == file) {
            return code_block;
        }
    }

    return nullptr;
}

void removeExistingElementInQueue(std::queue<std::pair<CodeBlock*, int>>* queue, CodeBlock* code_block) {
    std::queue<std::pair<CodeBlock*, int>> new_queue;

    while (!queue->empty()) {
        if (queue->front().first != code_block) {
            new_queue.push(std::make_pair(queue->front().first, queue->front().second));
        } else {
            std::cout << "Removing existing queue element: " << code_block->file->file_name << ", level: " << queue->front().second << "\n";
        }

        queue->pop();
    }

    *queue = new_queue;
}

void setFileImportLevels(vector<CodeBlock*> code_blocks) {
    std::queue<std::pair<CodeBlock*, int>> queue;

    for(CodeBlock* code_block : code_blocks) {
        if (isHeadFile(*code_block->file)) {
            // std::cout << "Head file: "<< code_block->file->file_name << "\n";
            queue.push(std::make_pair(code_block, 0));
        }
    }

    while (!queue.empty()) {
        auto item = queue.front();
        queue.pop();

        CodeBlock* code_block = item.first;
        code_block->import_level = item.second;

        for (Import* import: code_block->file->imports) {
            if (import->file == nullptr) { // Non-project import (e.g. "import os").
                continue;
            }

            removeExistingElementInQueue(&queue, code_block);

            CodeBlock* import_code_block = getCodeBlockByFile(import->file, code_blocks);
            queue.push(std::make_pair(import_code_block, code_block->import_level + 1));
        }
    }
}

void setStyle() {
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_MenuBarBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
    // colors[ImGuiCol_Menu] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    // colors[ImGuiCol_MenuSelectedBg] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
}

string getProjectPath() {
    nfdchar_t *path = nullptr;
    nfdresult_t result = NFD_PickFolder(nullptr, &path);

    if (result == NFD_OKAY) {
        std::cout << "Success!\n";
        std::cout << path << "\n";
    }
    else if (result == NFD_CANCEL) {
        std::cout << "User pressed cancel.\n";
    }
    else {
        std::cout << "Error: %s\n" << NFD_GetError() << "\n";
    }

    return path;
}

string getSettingsFile() {
    nfdchar_t *path = nullptr;
    nfdresult_t result = NFD_OpenDialog("json", nullptr, &path);

    if (result == NFD_OKAY) {
        std::cout << "Success!\n";
        std::cout << path << "\n";
    }
    else if (result == NFD_CANCEL) {
        std::cout << "User pressed cancel.\n";
    }
    else {
        std::cout << "Error: %s\n" << NFD_GetError() << "\n";
    }

    return path;
}

void moveGrid(ImVec2& grid_pos, ImVec2& grid_min_pos, ImVec2& grid_max_pos, bool recenter, float& zoom_level) {
    ImGuiIO& io = ImGui::GetIO();

    if (recenter) {
        grid_pos = ImVec2(0, 0);
        zoom_level = 3.0f;
    }

    if (ImGui::IsWindowHovered()) {
        static ImVec2 mouse_click_pos;

        if (ImGui::IsMouseDown(0)) {
            if (mouse_click_pos.x == 0 && mouse_click_pos.y == 0) {
                mouse_click_pos = ImGui::GetMousePos();
            }

            if (std::clamp(mouse_click_pos.x, grid_min_pos.x, grid_max_pos.x) != mouse_click_pos.x ||
                std::clamp(mouse_click_pos.y, grid_min_pos.y, grid_max_pos.y) != mouse_click_pos.y) {
                goto end;  // Mouse clicked outside of grid.
            }

            ImVec2 delta = io.MouseDelta;
            grid_pos.x += delta.x;
            grid_pos.y += delta.y;
        } else {
            mouse_click_pos = ImVec2(0, 0);
        }
        end:

        zoom_level += io.MouseWheel / 5;
        zoom_level = std::clamp(zoom_level, 0.25f, 1.75f);
    }
}

void drawBlocks(ImDrawList* draw_list, ImVec2& grid_pos, float zoom_level) {
    for (CodeBlock* code_block : MenuState.code_blocks) {
        code_block->drawBlock(draw_list, grid_pos, zoom_level);
        return;
    }
}

ImVec2 projectPosFromGrid(const ImVec2& pos_on_grid, const ImVec2& grid_pos, const ImVec2& grid_min_pos, float& margin, float& zoom_level) {
    ImVec2 screen_pos;

    

    return screen_pos;
}

void DrawGrid(float grid_step_size, ImU32 background_color, bool recenter) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 grid_min_pos = ImVec2(ImGui::GetColumnOffset(2) + 15, 65);
    ImVec2 grid_max_pos = ImVec2(MenuState.menu_width - 25, MenuState.menu_height - 25);

    draw_list->PushClipRect(grid_min_pos, grid_max_pos);
    draw_list->AddRectFilled(grid_min_pos,
                             ImVec2(grid_min_pos.x + MenuState.menu_width,
                                    grid_max_pos.y + MenuState.menu_height),
                             background_color);

    // Calculate the position of the grid based on the current mouse position
    static ImVec2 grid_pos = ImVec2(0, 0);

    static float zoom_level = 1.0f;
    moveGrid(grid_pos, grid_min_pos, grid_max_pos, recenter, zoom_level);

    grid_step_size = grid_step_size * zoom_level;

    float margin = 2.5f * grid_step_size;  // 50.0f

    grid_min_pos.x = grid_min_pos.x - (margin * zoom_level);
    grid_min_pos.y = grid_min_pos.y - (margin * zoom_level);

    grid_max_pos.x = grid_max_pos.x + (margin * zoom_level);
    grid_max_pos.y = grid_max_pos.y + (margin * zoom_level);


    // Loop over grid 'point', from which to draw 2 lines, down and to the right, to form the grid.
    for (int x = grid_min_pos.x; x < grid_max_pos.x; x += grid_step_size) {
        for (int y = grid_min_pos.y; y < grid_max_pos.y; y += grid_step_size) {
            // ImVec2 pos_on_grid = ImVec2(x * zoom_level - grid_pos.x, y * zoom_level - grid_pos.y);
            ImVec2 pos_on_grid = ImVec2(x + grid_pos.x + (margin * zoom_level),
                                        y + grid_pos.y + (margin * zoom_level));

            // Only draw the cell if it's within the mask area
            if (pos_on_grid.x >= grid_min_pos.x - margin && pos_on_grid.y >= grid_min_pos.y - zoom_level &&
                pos_on_grid.x < grid_min_pos.x + MenuState.menu_width && pos_on_grid.y < grid_min_pos.y + MenuState.menu_height) {

                draw_list->AddLine(pos_on_grid, ImVec2(pos_on_grid.x + grid_step_size, pos_on_grid.y), IM_COL32(255, 255, 255, 255));
                draw_list->AddLine(pos_on_grid, ImVec2(pos_on_grid.x, pos_on_grid.y + grid_step_size), IM_COL32(255, 255, 255, 255));
            }

            if (y > grid_min_pos.y) {
                std::cout << "2 Pos x: " << pos_on_grid.x << ", y: " << pos_on_grid.y << "\n\n";
                goto End;
            }
            std::cout << "1 Grid x: " << pos_on_grid.x << ", y: " << pos_on_grid.y << "\n";

            ImVec2 screen_pos = projectPosFromGrid();
            std::cout << "1 Screen x: " <<  << ", y: " << pos_on_grid.y << "\n";
        }
    }

    End:

    // std::cout << "\n          Min x: " << grid_min_pos.x - (margin * zoom_level) + grid_pos.x << ", Min y: " << grid_min_pos.y - (margin * zoom_level) + grid_pos.y << ", Max x: " << grid_max_pos.x - (margin * zoom_level) + grid_pos.x << ", Max y: " << grid_max_pos.y - (margin * zoom_level) + grid_pos.y << ".\n";

    drawBlocks(draw_list, grid_pos, zoom_level);

    draw_list->PopClipRect();
}

void drawMenuBar(vector<File*>& original_files) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open new project", "Ctrl + O")) {
                std::cout << "Open.\n";

                string project_path = getProjectPath();

                // TODO: Delete old file objects.

                original_files = openNewProject(project_path);
                MenuState.files = original_files;

                MenuState.selected_file_tab = 0;
            } else if (ImGui::MenuItem("Open project settings", "Ctrl + O")) {
                std::cout << "Open.\n";

                string settings_file = getSettingsFile();

                // TODO: Delete old file objects.

                original_files = loadDataFromJSON(settings_file);
                MenuState.files = original_files;

                MenuState.selected_file_tab = 0;
            } else if (ImGui::MenuItem("Save changes", "Ctrl + S")) {
                std::cout << "Save.\n";
                saveDataToJSON(MenuState.files, getProjectPath(MenuState.files[0]->file_name));
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void drawBody(vector<File*>& original_files) {
    ImGui::Columns(3);
    ImGui::SetColumnOffset(1, 200); {
        // Left: file select
        for(int i = 0; i < MenuState.files.size(); i++) {
            // for(File file: MenuState.files) {
            if (ImGui::MenuItem(MenuState.files[i]->file_name.c_str())) {
                MenuState.selected_file_tab = i;
            }
        }
    }
    ImGui::NextColumn(); {
        // Right: notes and todos
        File* selected_file = MenuState.files[MenuState.selected_file_tab];

        ImGui::Text("%s", selected_file->file_name.c_str());
        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        if (MenuState.selected_file_tab == 0) {
            if (ImGui::Button("Revert all changes")) {
                MenuState.files = original_files;
            }
        } else {
            ImGui::Dummy(ImVec2(0.0f, 19.0f));
        }

        if (ImGui::Button("Revert changes")) {
            MenuState.files[MenuState.selected_file_tab] = original_files[MenuState.selected_file_tab];
        }

        static char buffer[2048];
        strncpy(buffer, selected_file->notes.c_str(), sizeof(buffer));
        ImGui::InputTextMultiline("Notes", buffer, sizeof(buffer));

        if (string(buffer) != selected_file->notes) {
            MenuState.files[MenuState.selected_file_tab]->notes = buffer;
        }
    }
    ImGui::NextColumn(); {
        bool recenter = ImGui::Button("Recenter");
        DrawGrid(25.0f, IM_COL32(50, 50, 50, 255), recenter);
    }
}

void show(GLFWwindow* window, vector<File*>& original_files) {
    ImVec4 clear_color = ImVec4(0.125f, 0.00f, 0.25f, 0.00f);

    glfwPollEvents();

    glfwGetWindowSize(window, &MenuState.menu_width, &MenuState.menu_height);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
        // ImFont *font = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\verdana.ttf)", 16.0f);
        // ImGui::PushFont(font);  // TODO: Font.

        drawMenuBar(original_files);

        ImGui::SetNextWindowSize(ImVec2(MenuState.menu_width, MenuState.menu_height - 18));  // , ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 18));

        ImGui::Begin("Hello, world!", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        drawBody(original_files);

        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

void createWindow(vector<File*>& original_files) {
    static vector<CodeBlock*> code_blocks;
    code_blocks.reserve(original_files.size());

    for (File* file: original_files) {
        CodeBlock* code_block = new CodeBlock;
        code_block->file = file;

        code_blocks.push_back(code_block);
    }

    setFileImportLevels(code_blocks);

    MenuState.code_blocks = code_blocks;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        return;
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER , GLFW_TRUE);
    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(1560, 800, "Pypeline", nullptr, nullptr);
    if (window == nullptr) {
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    bool err = gladLoadGL() == 0;

    if (err) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();  // (void)io;

    setStyle();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Main loop
    MenuState.files = original_files;  // TODO: Add reset option.

    while (!glfwWindowShouldClose(window)) {
        show(window, original_files);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
#else
struct {
    int width;
    int height;
} MenuState;

void DrawGrid(ImDrawList* drawList, float grid_size) {
    ImVec2 mask_pos = ImVec2(50, 50);
    ImGuiIO& io = ImGui::GetIO();
    float length = 10.5f;

    drawList->PushClipRect(ImVec2(50, 50), ImVec2(MenuState.width * 0.8f, MenuState.height * 0.8f));

    // Calculate the position of the grid based on the current mouse position
    static ImVec2 gridPos = ImVec2(MenuState.width * length / 2, MenuState.height * length / 2);
    if (ImGui::IsWindowHovered()) {
        if (ImGui::IsMouseDown(0)) {
            ImVec2 delta = io.MouseDelta;
            gridPos.x -= delta.x;
            gridPos.y -= delta.y;
        }
    }

    std::cout << "X: " << gridPos.x << ", Y: " << gridPos.y << '\n';

    for (float x = 1.0f; x < MenuState.width * length; x += grid_size) {
        for (float y = 0.0f; y < MenuState.height * length; y += grid_size) {
            // Subtract the current grid position to make it movable
            ImVec2 pos = ImVec2(x - gridPos.x, y - gridPos.y);

            // Only draw the cell if it's within the mask area
            if (pos.x >= mask_pos.x && pos.y >= mask_pos.y &&
                pos.x < mask_pos.x + MenuState.width * 0.5f && pos.y < mask_pos.y + MenuState.height * 0.5f) {
                drawList->AddLine(pos, ImVec2(pos.x + grid_size, pos.y), IM_COL32(255, 255, 255, 255));
                drawList->AddLine(pos, ImVec2(pos.x, pos.y + grid_size), IM_COL32(255, 255, 255, 255));
            }
        }
    }

    drawList->PopClipRect();
}

void show() {
    ImGui::Begin("Hello, world!", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Set the clipping rectangle for the center of the window
    float grid_size = 50.0f;

    DrawGrid(drawList, grid_size);

    // End the window
    ImGui::End();
}

void createWindow(vector<File>& original_files) {
    ImVec4 clear_color = ImVec4(0.125f, 0.00f, 0.25f, 0.00f);

    // Initialize GLFW and create a window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        return;
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Grid Example", nullptr, nullptr);
    if (window == nullptr) {
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    bool err = gladLoadGL() == 0;

    if (err) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return;
    }

    // Initialize ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGui::StyleColorsDark();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glfwGetWindowSize(window, &MenuState.width, &MenuState.height);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            show();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
# endif