/**
 * @file UIManager.h
 * @brief Plik zrodlowy projektu Chess3D.
 */

#pragma once

#include <string>

struct GLFWwindow;

namespace chess3d {

class ChessGame;
class Renderer;
class Camera;

/// @brief Buduje interfejs ImGui do sterowania odtwarzaniem partii i parametrami sceny.
class UIManager {
public:
    UIManager() = default;
    ~UIManager();

    bool initialize(GLFWwindow* window);
    void shutdown();

    void beginFrame();
    void draw(ChessGame& game, Renderer& renderer, Camera& camera, float fps);
    void endFrame();

    // Did the UI consume mouse/keyboard input this frame?
    bool wantCaptureMouse() const;
    bool wantCaptureKeyboard() const;

private:
    void drawControlPanel(ChessGame& game, Renderer& renderer, Camera& camera, float fps);
    void drawMoveList    (ChessGame& game);
    void drawHelp        ();

    bool        initialized_  = false;
    char        pgnPathBuffer_[256] = {};
    std::string statusMessage_;
};

} // namespace chess3d

