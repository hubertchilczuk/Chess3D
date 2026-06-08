#include "Renderer.h"
#include "Camera.h"
#include "ChessGame.h"
#include "ChessPiece.h"

#include <iostream>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace chess3d {

namespace fs = std::filesystem;

Renderer::Renderer()  = default;
Renderer::~Renderer() = default;

bool Renderer::initialize() {
    // ---- Shaders ----
    const std::string shaderDir = CHESS3D_SHADERS_DIR;
    if (!pieceShader_.loadFromFiles(shaderDir + "/phong.vert",
                                    shaderDir + "/phong.frag")) {
        return false;
    }
    if (!boardShader_.loadFromFiles(shaderDir + "/phong.vert",
                                    shaderDir + "/phong.frag")) {
        return false;
    }

    // ---- Textures ----
    const std::string texDir = std::string(CHESS3D_RESOURCES_DIR) + "/textures";

    auto tryLoadOrProc = [&](Texture& dst, const std::string& filename,
                             bool light, bool wood) {
        std::string path = texDir + "/" + filename;
        if (fs::exists(path) && dst.loadFromFile(path)) {
            std::cout << "[Renderer] loaded texture: " << path << '\n';
            return;
        }
        if (wood) dst.createProceduralWood  (512, light);
        else      dst.createProceduralMarble(512, light);
    };
    tryLoadOrProc(lightWood_,   "wood_light.png",   true,  true);
    tryLoadOrProc(darkWood_,    "wood_dark.png",    false, true);
    tryLoadOrProc(whiteMarble_, "marble_white.png", true,  false);
    tryLoadOrProc(blackMarble_, "marble_black.png", false, false);

    // ---- Piece models ----
    // Expects a single combined file in resources/ or resources/models/
    // (no named groups) split into connected components by height.
    const std::string resDir   = CHESS3D_RESOURCES_DIR;
    const std::string modelDir = resDir + "/models";

    static constexpr const char* kCombinedExts[] = {
        ".obj", ".fbx", ".gltf", ".glb", ".dae", ".blend"
    };
    std::string combinedPath;
    for (const std::string& dir : { resDir, modelDir }) {
        if (!fs::exists(dir) || !fs::is_directory(dir)) continue;
        for (const auto& e : fs::directory_iterator(dir)) {
            if (!e.is_regular_file()) continue;
            std::string ext = e.path().extension().string();
            for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            bool isModelExt = false;
            for (const char* ce : kCombinedExts) if (ext == ce) { isModelExt = true; break; }
            if (!isModelExt) continue;

            std::string stem = e.path().stem().string();
            std::string lower = stem;
            for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (lower == "pawn"   || lower == "knight" || lower == "bishop" ||
                lower == "rook"   || lower == "queen"  || lower == "king") continue;

            combinedPath = e.path().string();
            break;
        }
        if (!combinedPath.empty()) break;
    }

    if (!combinedPath.empty()) {
        // Heights sorted ascending: pawn < rook < knight < bishop < queen < king
        static constexpr PieceType heightOrder[6] = {
            PieceType::Pawn, PieceType::Rook,  PieceType::Knight,
            PieceType::Bishop, PieceType::Queen, PieceType::King
        };
        auto comps = Model::loadAndSplitByComponents(combinedPath);
        if (comps.size() >= 6) {
            std::cout << "[Renderer] split " << comps.size()
                      << " connected components from "
                      << fs::path(combinedPath).filename().string() << '\n';
            for (int i = 0; i < 6; ++i)
                pieceModels_[static_cast<int>(heightOrder[i])] = std::move(comps[i]);
        } else {
            std::cerr << "[Renderer] combined file produced only "
                      << comps.size() << " components (need 6) - "
                         "some piece types will be missing.\n";
        }
    }

    // ---- Board geometry ----
    board_.build();

    // ---- Global GL state ----
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0.06f, 0.07f, 0.09f, 1.0f);
    return true;
}

void Renderer::resize(int w, int h) {
    width_  = std::max(1, w);
    height_ = std::max(1, h);
    glViewport(0, 0, width_, height_);
}

void Renderer::uploadCommonUniforms(Shader& sh, const Camera& camera, float aspect) {
    sh.bind();
    sh.setMat4("uView",       camera.view());
    sh.setMat4("uProjection", camera.projection(aspect));
    sh.setVec3("uViewPos",    camera.position());
    uploadLights(sh, camera);
}

void Renderer::uploadLights(Shader& sh, const Camera&) {
    sh.setVec3("uDirLight.direction", dirLight_.direction);
    sh.setVec3("uDirLight.ambient",   dirLight_.ambient);
    sh.setVec3("uDirLight.diffuse",   dirLight_.diffuse);
    sh.setVec3("uDirLight.specular",  dirLight_.specular);

    sh.setVec3 ("uPointLight.position",  pointLight_.position);
    sh.setVec3 ("uPointLight.ambient",   pointLight_.ambient);
    sh.setVec3 ("uPointLight.diffuse",   pointLight_.diffuse);
    sh.setVec3 ("uPointLight.specular",  pointLight_.specular);
    sh.setFloat("uPointLight.constant",  pointLight_.constant);
    sh.setFloat("uPointLight.linear",    pointLight_.linear);
    sh.setFloat("uPointLight.quadratic", pointLight_.quadratic);
}

void Renderer::drawBoard(const Camera& camera, const ChessGame& game, float aspect) {
    Shader& sh = boardShader_;
    uploadCommonUniforms(sh, camera, aspect);
    sh.setInt ("uDiffuseTex",        0);
    sh.setBool("uHasTexture",        true);
    sh.setVec3("uMaterialSpecular",  glm::vec3(0.20f));
    sh.setFloat("uMaterialShininess", 32.0f);

    // For "last move" highlight - light up source & destination of the latest move.
    Square hiFrom{-1,-1}, hiTo{-1,-1};
    if (game.hasLastMove()) {
        const Move& lm = game.lastMove();
        hiFrom = lm.from;
        hiTo   = lm.to;
    }

    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            bool light = ((f + r) & 1) != 0;
            if (light) lightWood_.bind(0);
            else       darkWood_ .bind(0);

            glm::mat4 model(1.0f);    // squares already in world space
            glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));

            sh.setMat4("uModel",         model);
            sh.setMat3("uNormalMatrix",  normalMatrix);
            sh.setVec3("uMaterialDiffuse", glm::vec3(1.0f));
            sh.setVec3("uTintColor",       glm::vec3(1.0f));

            float hl = board_.highlight(f, r);
            // boost highlight for last-move squares
            if ((Square{f, r} == hiFrom) || (Square{f, r} == hiTo)) {
                hl = std::max(hl, 0.65f);
            }
            sh.setFloat("uHighlight", hl);

            board_.squareMesh(f, r).draw();
        }
    }

    // Frame (dark wood, no highlight)
    darkWood_.bind(0);
    glm::mat4 model(1.0f);
    sh.setMat4("uModel",         model);
    sh.setMat3("uNormalMatrix",  glm::mat3(1.0f));
    sh.setVec3("uMaterialDiffuse", glm::vec3(0.55f));
    sh.setVec3("uTintColor",       glm::vec3(1.0f));
    sh.setFloat("uHighlight",      0.0f);
    board_.frameMesh().draw();

    // Labels engraved on the frame top. Flat quads with normal +Y so they're
    // double-sided-ish under typical lighting — disable culling just in case
    // the per-edge basis flips winding.
    sh.setBool("uHasTexture",        false);
    sh.setVec3("uMaterialDiffuse",   glm::vec3(0.96f, 0.92f, 0.78f));
    sh.setVec3("uMaterialSpecular",  glm::vec3(0.05f));
    sh.setFloat("uMaterialShininess", 8.0f);
    sh.setMat4("uModel",         model);
    sh.setMat3("uNormalMatrix",  glm::mat3(1.0f));
    sh.setVec3("uTintColor",     glm::vec3(1.0f));
    sh.setFloat("uHighlight",    0.0f);
    glDisable(GL_CULL_FACE);
    board_.labelsMesh().draw();
    glEnable(GL_CULL_FACE);
}

void Renderer::drawPieces(const Camera& camera, const ChessGame& game, float aspect) {
    Shader& sh = pieceShader_;
    uploadCommonUniforms(sh, camera, aspect);
    sh.setInt ("uDiffuseTex",         0);
    sh.setBool("uHasTexture",         true);
    sh.setVec3("uMaterialSpecular",   glm::vec3(0.55f));
    sh.setFloat("uMaterialShininess", 64.0f);

    // Highlight the piece currently animating (the most recent mover).
    const ChessPiece* recentMover = nullptr;
    for (const auto& up : game.pieces()) {
        if (up->animator().isPlaying()) {
            recentMover = up.get();
            break;
        }
    }

    for (const auto& up : game.pieces()) {
        const ChessPiece& p = *up;
        if (p.captured()) continue;
        Model* model = pieceModels_[static_cast<int>(p.type())].get();
        if (!model) continue;

        if (p.color() == Color::White) whiteMarble_.bind(0);
        else                            blackMarble_.bind(0);

        glm::vec3 pos = p.animator().currentPosition();
        glm::mat4 m(1.0f);
        m = glm::translate(m, pos);
        if (p.type() == PieceType::Knight) {
            // Orient knights so they face the opposing side.
            float rot = (p.color() == Color::White) ? 0.0f : glm::pi<float>();
            m = glm::rotate(m, rot, glm::vec3(0, 1, 0));
        }
        // Slight uniform scale so pieces fit comfortably inside a square.
        m = glm::scale(m, glm::vec3(0.95f));
        // Apply per-model normalisation last (identity for procedural pieces,
        // axis-fix + recenter + height-fit for imported Blender/FBX meshes).
        m = m * model->normalizationTransform();

        glm::mat3 nm = glm::mat3(glm::transpose(glm::inverse(m)));
        sh.setMat4("uModel",        m);
        sh.setMat3("uNormalMatrix", nm);
        sh.setVec3("uMaterialDiffuse", glm::vec3(1.0f));

        // Color tint
        glm::vec3 tint = (p.color() == Color::White)
            ? glm::vec3(1.00f, 0.98f, 0.92f)
            : glm::vec3(0.45f, 0.40f, 0.38f);
        sh.setVec3("uTintColor", tint);

        float hl = (&p == recentMover) ? 0.45f : 0.0f;
        sh.setFloat("uHighlight", hl);

        model->draw();
    }
}

void Renderer::render(const Camera& camera, const ChessGame& game, float /*time*/) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (height_ <= 0) return;
    float aspect = static_cast<float>(width_) / static_cast<float>(height_);

    drawBoard (camera, game, aspect);
    drawPieces(camera, game, aspect);
}

} // namespace chess3d
