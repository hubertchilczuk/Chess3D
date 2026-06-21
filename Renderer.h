/**
 * @file Renderer.h
 * @brief Plik zrodlowy projektu Chess3D.
 */

#pragma once

#include "Shader.h"
#include "Texture.h"
#include "Light.h"
#include "Model.h"
#include "ChessBoard.h"
#include "Common.h"
#include <array>
#include <memory>

namespace chess3d {

class Camera;
class ChessGame;

/// @brief Renderuje planszę, figury, światła i materiały sceny 3D.
class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize();
    void resize(int width, int height);

    void render(const Camera& camera, const ChessGame& game, float time);

    ChessBoard&       board()       { return board_; }
    const ChessBoard& board() const { return board_; }

    DirectionalLight& dirLight()     { return dirLight_;   }
    PointLight&       pointLight()   { return pointLight_; }

private:
    void drawBoard   (const Camera& camera, const ChessGame& game, float aspect);
    void drawPieces  (const Camera& camera, const ChessGame& game, float aspect);
    void uploadCommonUniforms(Shader& sh, const Camera& camera, float aspect);
    void uploadLights        (Shader& sh, const Camera& camera);

    // OpenGL resources
    Shader      pieceShader_;
    Shader      boardShader_;
    Texture     lightWood_;
    Texture     darkWood_;
    Texture     whiteMarble_;
    Texture     blackMarble_;
    std::array<std::unique_ptr<Model>, 7> pieceModels_; // indexed by PieceType (None unused)

    // Scene
    ChessBoard       board_;
    DirectionalLight dirLight_;
    PointLight       pointLight_;

    int width_  = 1;
    int height_ = 1;
};

} // namespace chess3d

