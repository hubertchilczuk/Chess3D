#include "PGNParser.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <algorithm>

namespace chess3d {

namespace {

std::string ltrim(std::string s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(),
                         [](unsigned char c) { return !std::isspace(c); }));
    return s;
}
std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char c) { return !std::isspace(c); }).base(),
            s.end());
    return s;
}
std::string trim(std::string s) { return rtrim(ltrim(std::move(s))); }

bool isResult(const std::string& tok) {
    return tok == "1-0" || tok == "0-1" || tok == "1/2-1/2" || tok == "*";
}

} // anon

std::string PGNParser::stripComments(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    int  braceDepth = 0;
    int  parenDepth = 0;
    bool lineComment = false;

    for (char c : text) {
        if (lineComment) {
            if (c == '\n' || c == '\r') lineComment = false;
            continue;
        }
        if (c == ';') { lineComment = true; continue; }
        if (c == '{') { ++braceDepth; continue; }
        if (c == '}') { if (braceDepth) --braceDepth; continue; }
        if (c == '(') { ++parenDepth; continue; }
        if (c == ')') { if (parenDepth) --parenDepth; continue; }
        if (braceDepth || parenDepth) continue;
        out.push_back(c);
    }
    return out;
}

bool PGNParser::parseSAN(const std::string& sanIn, Color toMove, Move& out) {
    out = Move{};
    out.color = toMove;

    std::string s = sanIn;

    // Strip annotation suffixes (!, ?, !!, ??, !?, ?!)
    while (!s.empty() && (s.back() == '!' || s.back() == '?')) s.pop_back();
    if (s.empty()) return false;

    // Check / mate suffix
    if (s.back() == '#') { out.isMate  = true; s.pop_back(); }
    else if (s.back() == '+') { out.isCheck = true; s.pop_back(); }
    if (s.empty()) return false;

    // Castling
    if (s == "O-O" || s == "0-0") {
        out.isCastleKingside = true;
        out.piece            = PieceType::King;
        out.san              = sanIn;
        return true;
    }
    if (s == "O-O-O" || s == "0-0-0") {
        out.isCastleQueenside = true;
        out.piece             = PieceType::King;
        out.san               = sanIn;
        return true;
    }

    // Promotion ("=Q" or just trailing letter on some old PGNs)
    auto eq = s.find('=');
    if (eq != std::string::npos && eq + 1 < s.size()) {
        out.promotion = pieceFromLetter(s[eq + 1]);
        s.erase(eq);
    } else if (s.size() >= 3) {
        // tolerate "e8Q" - last char uppercase piece letter, second-to-last digit
        char c1 = s[s.size() - 1];
        char c2 = s[s.size() - 2];
        if (std::isupper(static_cast<unsigned char>(c1)) && std::isdigit(static_cast<unsigned char>(c2))
            && (c1 == 'Q' || c1 == 'R' || c1 == 'B' || c1 == 'N')) {
            out.promotion = pieceFromLetter(c1);
            s.pop_back();
        }
    }

    if (s.size() < 2) return false;

    // Destination = last 2 chars  [a-h][1-8]
    char destR = s.back(); s.pop_back();
    char destF = s.back(); s.pop_back();
    if (destF < 'a' || destF > 'h' || destR < '1' || destR > '8') return false;
    out.to = { destF - 'a', destR - '1' };

    // Capture marker
    if (!s.empty() && s.back() == 'x') {
        out.isCapture = true;
        s.pop_back();
    }

    // Piece type (first remaining char if uppercase)
    if (!s.empty() && std::isupper(static_cast<unsigned char>(s.front()))) {
        out.piece = pieceFromLetter(s.front());
        s.erase(0, 1);
    } else {
        out.piece = PieceType::Pawn;
    }

    // Remaining chars (0..2) are disambiguation hints.
    for (char c : s) {
        if (c >= 'a' && c <= 'h') out.from.file = c - 'a';
        else if (c >= '1' && c <= '8') out.from.rank = c - '1';
        // ignore anything else silently
    }

    out.san = sanIn;
    return true;
}

PGNParser::ParseResult PGNParser::parseString(const std::string& text) {
    ParseResult result;

    // Split headers and movetext.
    std::string moveText;
    {
        std::istringstream iss(text);
        std::string line;
        while (std::getline(iss, line)) {
            std::string t = trim(line);
            if (t.empty()) { moveText += '\n'; continue; }
            if (t.front() == '[' && t.back() == ']') {
                // Header: [Key "Value"]
                auto sp = t.find(' ');
                auto q1 = t.find('"');
                auto q2 = t.rfind('"');
                if (sp != std::string::npos && q1 != std::string::npos && q2 > q1) {
                    result.headers.emplace_back(
                        t.substr(1, sp - 1),
                        t.substr(q1 + 1, q2 - q1 - 1));
                }
            } else {
                moveText += t;
                moveText += ' ';
            }
        }
    }

    moveText = stripComments(moveText);

    std::istringstream toks(moveText);
    std::string tok;
    Color toMove = Color::White;

    while (toks >> tok) {
        if (tok.empty()) continue;
        if (isResult(tok)) break;

        // Skip NAGs ($1, $14, ...).
        if (tok.front() == '$') continue;

        // Strip leading move numbers like "1." or "12..."
        if (std::isdigit(static_cast<unsigned char>(tok.front()))) {
            auto dot = tok.find('.');
            if (dot != std::string::npos) {
                std::string rest = tok.substr(dot + 1);
                while (!rest.empty() && rest.front() == '.') rest.erase(rest.begin());
                if (rest.empty()) continue;     // pure move number, e.g. "1."
                tok = rest;
            } else {
                // Bare number? Skip.
                continue;
            }
        }

        if (tok.empty() || isResult(tok)) {
            if (!tok.empty() && isResult(tok)) break;
            continue;
        }

        Move m;
        if (!parseSAN(tok, toMove, m)) {
            result.error = "Cannot parse SAN token: \"" + tok + "\"";
            return result;
        }
        result.moves.push_back(std::move(m));
        toMove = opposite(toMove);
    }

    return result;
}

PGNParser::ParseResult PGNParser::parseFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        ParseResult r;
        r.error = "Cannot open file: " + path;
        return r;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return parseString(ss.str());
}

} // namespace chess3d
