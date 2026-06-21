/**
 * @file PGNParser.h
 * @brief Plik zrodlowy projektu Chess3D.
 */

#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <utility>

namespace chess3d {

// Pure parsing -- knows nothing about a board. It produces a list of
// Move records with `piece` and `to` resolved from the SAN, and any
// disambiguation hints stored in `from.file` / `from.rank`
// (-1 = unspecified). ChessGame is responsible for resolving the
// actual source square using the live board state, because that
// requires knowing where pieces currently are.
class PGNParser {
public:
    struct ParseResult {
        std::vector<std::pair<std::string, std::string>> headers;
        std::vector<Move> moves;
        std::string error;
    };

    static ParseResult parseFile  (const std::string& path);
    static ParseResult parseString(const std::string& text);

    // Helper exposed for testing & for the ChessGame resolver.
    static bool parseSAN(const std::string& san, Color toMove, Move& out);

private:
    static std::string stripComments(const std::string& text);
};

} // namespace chess3d

