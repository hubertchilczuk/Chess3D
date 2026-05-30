#include "UIManager.h"
#include "ChessGame.h"
#include "Renderer.h"
#include "Camera.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <cstring>
#include <cstdio>
#include <filesystem>

namespace chess3d {

UIManager::~UIManager() { shutdown(); }

bool UIManager::initialize(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;     // no imgui.ini side files
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))  return false;
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) return false;

    std::snprintf(pgnPathBuffer_, sizeof(pgnPathBuffer_),
                  "%s/pgn/sample_game.pgn", CHESS3D_RESOURCES_DIR);
    initialized_ = true;
    return true;
}

void UIManager::shutdown() {
    if (!initialized_) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
}

void UIManager::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::draw(ChessGame& game, Renderer& renderer, Camera& camera, float fps) {
    drawControlPanel(game, renderer, camera, fps);
    drawMoveList    (game);
    drawHelp        ();
}

void UIManager::drawControlPanel(ChessGame& game, Renderer& renderer, Camera& camera, float fps) {
    ImGui::SetNextWindowPos (ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("Chess3D - Control");

    ImGui::Text("FPS: %.1f", static_cast<double>(fps));

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f), "Game");
    if (!game.pgnHeader("White").empty()) {
        ImGui::Text("White : %s", game.pgnHeader("White").c_str());
    }
    if (!game.pgnHeader("Black").empty()) {
        ImGui::Text("Black : %s", game.pgnHeader("Black").c_str());
    }
    if (!game.pgnHeader("Event").empty()) {
        ImGui::Text("Event : %s", game.pgnHeader("Event").c_str());
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f), "Playback");
    ImGui::Text("Move %d / %d", game.currentMoveIndex(), game.totalMoves());

    if (game.isPlaying()) {
        if (ImGui::Button("Pause")) game.pause();
    } else {
        if (ImGui::Button("Play"))  game.play();
    }
    ImGui::SameLine();
    if (ImGui::Button("<< Prev")) game.stepBackward();
    ImGui::SameLine();
    if (ImGui::Button("Next >>")) game.stepForward();
    ImGui::SameLine();
    if (ImGui::Button("Reset"))   { game.resetToStartingPosition(); }

    float spd = game.speed();
    if (ImGui::SliderFloat("Speed (mv/s)", &spd, 0.1f, 4.0f, "%.2f")) {
        game.setSpeed(spd);
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f), "PGN");
    ImGui::InputText("Path", pgnPathBuffer_, sizeof(pgnPathBuffer_));
    if (ImGui::Button("Load PGN")) {
        if (game.loadPGNFile(pgnPathBuffer_)) {
            statusMessage_ = "Loaded " +
                std::filesystem::path(pgnPathBuffer_).filename().string();
        } else {
            statusMessage_ = "Failed to load PGN";
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload sample")) {
        std::string p = std::string(CHESS3D_RESOURCES_DIR) + "/pgn/sample_game.pgn";
        std::strncpy(pgnPathBuffer_, p.c_str(), sizeof(pgnPathBuffer_) - 1);
        if (game.loadPGNFile(p)) statusMessage_ = "Loaded sample_game.pgn";
    }
    if (!statusMessage_.empty()) {
        ImGui::TextWrapped("%s", statusMessage_.c_str());
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f), "Lighting");
    ImGui::DragFloat3("Dir light direction", &renderer.dirLight().direction.x, 0.01f, -1.0f, 1.0f);
    ImGui::ColorEdit3("Dir diffuse",         &renderer.dirLight().diffuse.x);
    ImGui::DragFloat3("Point light pos",     &renderer.pointLight().position.x, 0.05f);
    ImGui::ColorEdit3("Point diffuse",       &renderer.pointLight().diffuse.x);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.9f, 0.85f, 0.4f, 1.0f), "Camera");
    ImGui::Text("Pos: %.1f, %.1f, %.1f",
                static_cast<double>(camera.position().x),
                static_cast<double>(camera.position().y),
                static_cast<double>(camera.position().z));
    ImGui::DragFloat("Move speed", &camera.moveSpeed, 0.1f, 0.5f, 30.0f);
    ImGui::DragFloat("Mouse sens", &camera.mouseSensitivity, 0.01f, 0.01f, 1.0f);

    ImGui::End();
}

void UIManager::drawMoveList(ChessGame& game) {
    ImGui::SetNextWindowPos (ImVec2(10, 440), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin("Moves");

    const auto& moves = game.moves();
    if (ImGui::BeginTable("moves_table", 3,
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
            ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("#",     ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("White");
        ImGui::TableSetupColumn("Black");
        ImGui::TableHeadersRow();

        int rows = (static_cast<int>(moves.size()) + 1) / 2;
        for (int row = 0; row < rows; ++row) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d.", row + 1);

            int wIdx = row * 2;
            int bIdx = row * 2 + 1;
            ImGui::TableSetColumnIndex(1);
            if (wIdx < static_cast<int>(moves.size())) {
                bool current = (game.currentMoveIndex() == wIdx + 1);
                if (current) ImGui::TextColored(ImVec4(1, 0.85f, 0.3f, 1), "%s", moves[wIdx].san.c_str());
                else         ImGui::TextUnformatted(moves[wIdx].san.c_str());
            }
            ImGui::TableSetColumnIndex(2);
            if (bIdx < static_cast<int>(moves.size())) {
                bool current = (game.currentMoveIndex() == bIdx + 1);
                if (current) ImGui::TextColored(ImVec4(1, 0.85f, 0.3f, 1), "%s", moves[bIdx].san.c_str());
                else         ImGui::TextUnformatted(moves[bIdx].san.c_str());
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void UIManager::drawHelp() {
    ImGui::SetNextWindowPos (ImVec2(1280 - 290, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 200),      ImGuiCond_FirstUseEver);
    ImGui::Begin("Help");
    ImGui::TextWrapped("Camera:");
    ImGui::BulletText("WASD - move");
    ImGui::BulletText("Q / E - down / up");
    ImGui::BulletText("RMB + drag - look");
    ImGui::BulletText("Wheel - zoom");
    ImGui::Separator();
    ImGui::TextWrapped("Playback:");
    ImGui::BulletText("Space - play/pause");
    ImGui::BulletText("Left / Right - prev / next");
    ImGui::BulletText("R - reset position");
    ImGui::End();
}

bool UIManager::wantCaptureMouse()    const { return initialized_ && ImGui::GetIO().WantCaptureMouse;    }
bool UIManager::wantCaptureKeyboard() const { return initialized_ && ImGui::GetIO().WantCaptureKeyboard; }

} // namespace chess3d
