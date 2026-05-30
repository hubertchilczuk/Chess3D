#include "Application.h"
#include <iostream>
#include <exception>

int main(int /*argc*/, char** /*argv*/) {
    try {
        chess3d::Application app;
        if (!app.initialize(1280, 800, "Chess3D - PGN Viewer")) {
            std::cerr << "[Chess3D] Application failed to initialize.\n";
            return EXIT_FAILURE;
        }
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "[Chess3D] Fatal: " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "[Chess3D] Fatal: unknown exception\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
