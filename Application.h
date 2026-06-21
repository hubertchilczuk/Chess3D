/**
 * @file Application.h
 * @brief Plik zrodlowy projektu Chess3D.
 */

#pragma once

#include <memory>
#include <string>

struct GLFWwindow;

namespace chess3d {

class Renderer;
class Camera;
class ChessGame;
class UIManager;

/// @brief Główna klasa aplikacji odpowiedzialna za okno, pętlę programu i integrację modułów.
class Application {
public:
    Application();
    ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    bool initialize(int width, int height, const std::string& title);
    void run();
    void shutdown();

    // GLFW callbacks (forwarded by free functions)
    void onResize(int width, int height);
    void onMouseButton(int button, int action);
    void onMouseMove(double xpos, double ypos);
    void onScroll(double yoffset);
    void onKey(int key, int action);

private:
    void processInput(float dt);
    void update(float dt);
    void render();

    GLFWwindow* window_ = nullptr;
    int   width_  = 0;
    int   height_ = 0;

    std::unique_ptr<Renderer>  renderer_;
    std::unique_ptr<Camera>    camera_;
    std::unique_ptr<ChessGame> game_;
    std::unique_ptr<UIManager> ui_;

    // input state
    double lastMouseX_      = 0.0;
    double lastMouseY_      = 0.0;
    bool   firstMouse_      = true;
    bool   rightMouseDown_  = false;

    // timing
    float lastFrameTime_ = 0.0f;
    float fpsAccum_      = 0.0f;
    int   fpsFrames_     = 0;
    float lastFps_       = 0.0f;
};

} // namespace chess3d

