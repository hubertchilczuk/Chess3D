cmake_minimum_required(VERSION 3.20)
project(Chess3D LANGUAGES CXX C)

# Some third-party FetchContent dependencies still ship with very old
# cmake_minimum_required (< 3.5). CMake 4.x removed compatibility with
# them by default, so we tell it to keep the old behaviour for sub-projects.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W3 /permissive- /Zc:__cplusplus /MP)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# ---------- GLFW ----------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.9
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(glfw)

# ---------- GLM ----------
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(glm)

# ---------- Assimp ----------
set(ASSIMP_BUILD_TESTS              OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES            OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS       OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL                  OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX     OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT                ON  CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS       OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER       ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER       ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER   ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER      ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_PLY_IMPORTER       ON  CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS               OFF CACHE BOOL "" FORCE)
FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG        v5.4.2
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(assimp)

# ---------- GLEW (loader) ----------
# We use glew-cmake (Perlmint fork) since it ships with CMake support
# and does NOT require Python (vs. Dav1dde/glad which does).
set(glew-cmake_BUILD_SHARED      OFF CACHE BOOL "" FORCE)
set(glew-cmake_BUILD_STATIC      ON  CACHE BOOL "" FORCE)
set(glew-cmake_BUILD_MULTI_CONTEXT OFF CACHE BOOL "" FORCE)
set(ONLY_LIBS                    ON  CACHE BOOL "" FORCE)
FetchContent_Declare(glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG        glew-cmake-2.2.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(glew)

# ---------- stb ----------
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(stb)

# ---------- ImGui ----------
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw)

# ---------- Chess3D ----------
file(GLOB CHESS3D_SRC  CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB CHESS3D_HDR  CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/*.h")

add_executable(Chess3D ${CHESS3D_SRC} ${CHESS3D_HDR})

target_include_directories(Chess3D PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${stb_SOURCE_DIR})

target_link_libraries(Chess3D PRIVATE
    glfw
    glm::glm
    libglew_static
    assimp
    imgui)

target_compile_definitions(Chess3D PRIVATE
    CHESS3D_RESOURCES_DIR="${CMAKE_SOURCE_DIR}/resources"
    CHESS3D_SHADERS_DIR="${CMAKE_SOURCE_DIR}/shaders"
    GLEW_STATIC
    GLFW_INCLUDE_NONE)

set_target_properties(Chess3D PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

if(MSVC)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR}
        PROPERTY VS_STARTUP_PROJECT Chess3D)
endif()

source_group("Sources"  FILES ${CHESS3D_SRC})
source_group("Headers"  FILES ${CHESS3D_HDR})

message(STATUS "Chess3D configured. Resources: ${CMAKE_SOURCE_DIR}/resources")
