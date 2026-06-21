/**
 * @file Application.cpp
 * @brief Plik zrodlowy projektu Chess3D.
 */

#include "Application.h"

#include "Renderer.h"
#include "Camera.h"
#include "ChessGame.h"
#include "UIManager.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <filesystem>

namespace chess3d {

namespace {

void glfwErrorCallback(int code, const char* desc) {
    std::cerr << "[GLFW] error " << code << ": " << desc << '\n';
}

Application* fromWindow(GLFWwindow* w) {
    return static_cast<Application*>(glfwGetWindowUserPointer(w));
}

void cbResize(GLFWwindow* w, int width, int height) {
    if (auto* app = fromWindow(w)) app->onResize(width, height);
}
void cbMouseButton(GLFWwindow* w, int button, int action, int /*mods*/) {
    if (auto* app = fromWindow(w)) app->onMouseButton(button, action);
}
void cbCursorPos(GLFWwindow* w, double x, double y) {
    if (auto* app = fromWindow(w)) app->onMouseMove(x, y);
}
void cbScroll(GLFWwindow* w, double /*xo*/, double yo) {
    if (auto* app = fromWindow(w)) app->onScroll(yo);
}
void cbKey(GLFWwindow* w, int key, int /*sc*/, int action, int /*mods*/) {
    if (auto* app = fromWindow(w)) app->onKey(key, action);
}

} // anon

Application::Application()  = default;
Application::~Application() { shutdown(); }

bool Application::initialize(int width, int height, const std::string& title) {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "[Chess3D] glfwInit failed\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES,               4);

    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window_) {
        std::cerr << "[Chess3D] glfwCreateWindow failed\n";
        glfwTerminate();
        return false;
    }
    width_  = width;
    height_ = height;

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(window_, this);

    glfwSetFramebufferSizeCallback(window_, cbResize);
    glfwSetMouseButtonCallback    (window_, cbMouseButton);
    glfwSetCursorPosCallback      (window_, cbCursorPos);
    glfwSetScrollCallback         (window_, cbScroll);
    glfwSetKeyCallback            (window_, cbKey);

    glewExperimental = GL_TRUE;
    if (GLenum err = glewInit(); err != GLEW_OK) {
        std::cerr << "[Chess3D] glewInit failed: "
                  << reinterpret_cast<const char*>(glewGetErrorString(err)) << '\n';
        return false;
    }
    glGetError(); // GLEW sometimes leaves a benign INVALID_ENUM on init.
    glEnable(GL_MULTISAMPLE);

    std::cout << "[Chess3D] OpenGL " << glGetString(GL_VERSION)
              << " | Renderer: " << glGetString(GL_RENDERER) << '\n';

    renderer_ = std::make_unique<Renderer>();
    if (!renderer_->initialize()) {
        std::cerr << "[Chess3D] renderer initialization failed\n";
        return false;
    }
    int fbW, fbH;
    glfwGetFramebufferSize(window_, &fbW, &fbH);
    renderer_->resize(fbW, fbH);

    camera_ = std::make_unique<Camera>(glm::vec3(0.0f, 7.5f, 9.0f), -90.0f, -42.0f);
    camera_->lookAt(glm::vec3(0.0f, 0.0f, 0.0f));

    game_ = std::make_unique<ChessGame>();
    {
        std::string defaultPgn = std::string(CHESS3D_RESOURCES_DIR) + "/pgn/sample_game.pgn";
        if (std::filesystem::exists(defaultPgn)) {
            game_->loadPGNFile(defaultPgn);
        }
    }

    ui_ = std::make_unique<UIManager>();
    if (!ui_->initialize(window_)) {
        std::cerr << "[Chess3D] UI manager init failed\n";
        return false;
    }

    lastFrameTime_ = static_cast<float>(glfwGetTime());
    return true;
}

void Application::shutdown() {
    ui_.reset();
    renderer_.reset();
    camera_.reset();
    game_.reset();
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void Application::onResize(int width, int height) {
    width_  = std::max(1, width);
    height_ = std::max(1, height);
    if (renderer_) renderer_->resize(width_, height_);
}

void Application::onMouseButton(int button, int action) {
    if (ui_ && ui_->wantCaptureMouse()) return;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        rightMouseDown_ = (action == GLFW_PRESS);
        if (rightMouseDown_) {
            firstMouse_ = true;
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void Application::onMouseMove(double xpos, double ypos) {
    if (firstMouse_) {
        lastMouseX_ = xpos;
        lastMouseY_ = ypos;
        firstMouse_ = false;
        return;
    }
    float dx = static_cast<float>(xpos - lastMouseX_);
    float dy = static_cast<float>(ypos - lastMouseY_);
    lastMouseX_ = xpos;
    lastMouseY_ = ypos;

    if (rightMouseDown_ && camera_) camera_->processMouse(dx, dy);
}

void Application::onScroll(double yoffset) {
    if (ui_ && ui_->wantCaptureMouse()) return;
    if (camera_) camera_->processScroll(static_cast<float>(yoffset));
}

void Application::onKey(int key, int action) {
    if (ui_ && ui_->wantCaptureKeyboard()) return;
    if (action != GLFW_PRESS) return;
    switch (key) {
        case GLFW_KEY_ESCAPE:    glfwSetWindowShouldClose(window_, GLFW_TRUE); break;
        case GLFW_KEY_SPACE:     if (game_) game_->togglePlay();               break;
        case GLFW_KEY_RIGHT:     if (game_) game_->stepForward();              break;
        case GLFW_KEY_LEFT:      if (game_) game_->stepBackward();             break;
        case GLFW_KEY_R:         if (game_) game_->resetToStartingPosition();  break;
        default: break;
    }
}

void Application::processInput(float dt) {
    if (!camera_) return;
    if (ui_ && ui_->wantCaptureKeyboard()) return;

    glm::vec3 dir(0.0f);
    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) dir.z += 1.0f;
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) dir.z -= 1.0f;
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) dir.x += 1.0f;
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) dir.x -= 1.0f;
    if (glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS) dir.y += 1.0f;
    if (glfwGetKey(window_, GLFW_KEY_Q) == GLFW_PRESS) dir.y -= 1.0f;

    if (glm::length(dir) > 0.0f) camera_->processKeyboard(dir, dt);
}

void Application::update(float dt) {
    if (game_)     game_->update(dt);
    if (renderer_) renderer_->board().update(dt);
}

void Application::render() {
    renderer_->render(*camera_, *game_, static_cast<float>(glfwGetTime()));

    ui_->beginFrame();
    ui_->draw(*game_, *renderer_, *camera_, lastFps_);
    ui_->endFrame();

    glfwSwapBuffers(window_);
}

void Application::run() {
    while (!glfwWindowShouldClose(window_)) {
        float now = static_cast<float>(glfwGetTime());
        float dt  = now - lastFrameTime_;
        if (dt > 0.1f) dt = 0.1f;     // cap large frame steps
        lastFrameTime_ = now;

        // FPS accumulator
        fpsAccum_  += dt;
        fpsFrames_ += 1;
        if (fpsAccum_ >= 0.5f) {
            lastFps_ = static_cast<float>(fpsFrames_) / fpsAccum_;
            fpsAccum_  = 0.0f;
            fpsFrames_ = 0;
        }

        glfwPollEvents();
        processInput(dt);
        update(dt);
        render();
    }
}

} // namespace chess3d

