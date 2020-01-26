/*****************************************************************************************
 *                                                                                       *
 * Textured OBJ Renderer                                                                 *
 *                                                                                       *
 * Copyright (c) Alexander Bock, 2020                                                    *
 *                                                                                       *
 * All rights reserved.                                                                  *
 *                                                                                       *
 * Redistribution and use in source and binary forms, with or without modification, are  *
 * permitted provided that the following conditions are met:                             *
 *                                                                                       *
 * 1. Redistributions of source code must retain the above copyright notice, this list   *
 *    of conditions and the following disclaimer.                                        *
 *                                                                                       *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this     *
 *    list of conditions and the following disclaimer in the documentation and/or other  *
 *    materials provided with the distribution.                                          *
 *                                                                                       *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY   *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES  *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT   *
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,        *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  *
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR    *
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN      *
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN    *
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
 * DAMAGE.                                                                               *
 ****************************************************************************************/

#include "objloader.h"

#include <sgct/log.h>
#include <glm/glm.hpp>
#include <charconv>
#include <fstream>
#include <functional>
#include <sstream>

namespace {
    constexpr const char* IgnoredTokens[] = {
        "mtllib",
        "o",
        "usemtl",
        "s"
    };

    enum class Token {
        Vertex,
        Normal,
        UV,
        Face,
        
        Ignored,
        Unknown
    };

    Token tokenize(std::string_view token) {
        if (token == "v") {
            return Token::Vertex;
        }
        else if (token == "vn") {
            return Token::Normal;
        }
        else if (token == "vt") {
            return Token::UV;
        }
        else if (token == "f") {
            return Token::Face;
        }
        auto it = std::find(std::cbegin(IgnoredTokens), std::cend(IgnoredTokens), token);
        if (it != std::cend(IgnoredTokens)) {
            return Token::Ignored;
        }

        return Token::Unknown;
    }

    glm::vec2 readVec2(const std::string& x, const std::string& y) {
        glm::vec3 r;
#ifdef WIN32
        std::from_chars_result xRes = std::from_chars(x.data(), x.data() + x.size(), r.x);
        std::from_chars_result yRes = std::from_chars(y.data(), y.data() + y.size(), r.y);

        if (xRes.ec != std::errc() || yRes.ec != std::errc()) {
            throw std::runtime_error("Error loading line");
        }
#else // WIN32
        std::istringstream xStream(x);
        xStream >> r.x;

        std::istringstream yStream(y);
        yStream >> r.y;

        if (xStream.fail() || yStream.fail()) {
            throw std::runtime_error("Error loading line");
        }
#endif // WIN32

        return r;
    }

    glm::vec3 readVec3(const std::string& x, const std::string& y, const std::string& z) {
        glm::vec3 r;
#ifdef WIN32
        std::from_chars_result xRes = std::from_chars(x.data(), x.data() + x.size(), r.x);
        std::from_chars_result yRes = std::from_chars(y.data(), y.data() + y.size(), r.y);
        std::from_chars_result zRes = std::from_chars(z.data(), z.data() + z.size(), r.z);

        if (xRes.ec != std::errc() || yRes.ec != std::errc() ||
            zRes.ec != std::errc())
        {
            throw std::runtime_error("Error loading line");
        }
#else // WIN32
        std::istringstream xStream(x);
        xStream >> r.x;

        std::istringstream yStream(y);
        yStream >> r.y;

        std::istringstream zStream(z);
        zStream >> r.z;

        if (xStream.fail() || yStream.fail() || zStream.fail()) {
            throw std::runtime_error("Error loading line");
        }
#endif // WIN32

        return r;
    }


    obj::Position readPosition(std::string_view line) {
        const size_t firstSeparator = line.find(' ');
        const size_t secondSeparator = line.find(' ', firstSeparator + 1);

        std::string xStr = std::string(line.substr(0, firstSeparator));
        std::string yStr = std::string(
            line.substr(firstSeparator + 1, secondSeparator - firstSeparator - 1)
        );
        std::string zStr = std::string(line.substr(secondSeparator + 1));

        obj::Position res;
        glm::vec3 r = readVec3(xStr, yStr, zStr);
        res.x = r.x;
        res.y = r.y;
        res.z = r.z;
        return res;
    }

    obj::Normal readNormal(std::string_view line) {
        const size_t firstSeparator = line.find(' ');
        const size_t secondSeparator = line.find(' ', firstSeparator + 1);

        std::string xStr = std::string(line.substr(0, firstSeparator));
        std::string yStr = std::string(
            line.substr(firstSeparator + 1, secondSeparator - firstSeparator - 1)
        );
        std::string zStr = std::string(line.substr(secondSeparator + 1));

        obj::Normal res;
        glm::vec3 r = readVec3(xStr, yStr, zStr);
        res.nx = r.x;
        res.ny = r.y;
        res.nz = r.z;
        return res;
    }

    obj::UV readUV(std::string_view line) {
        const size_t separator = line.find(' ');

        std::string uStr = std::string(line.substr(0, separator));
        std::string vStr = std::string(line.substr(separator + 1));

        obj::UV res;
        glm::vec2 r = readVec2(uStr, vStr);
        res.u = r.x;
        res.v = r.y;
        return res;
    }

    obj::Face readFace(std::string_view line) {
        const size_t firstSep = line.find(' ');
        const size_t secondSep = line.find(' ', firstSep + 1);
        const size_t thirdSep = line.find(' ', secondSep + 1);
        const bool hasFourth = thirdSep != std::string_view::npos;

        std::string_view i0Str = line.substr(0, firstSep);
        std::string_view i1Str = line.substr(
            firstSep + 1,
            secondSep - firstSep - 1
        );
        std::string_view i2Str = hasFourth ?
            line.substr(secondSep + 1, thirdSep - secondSep - 1) :
            line.substr(secondSep + 1);
        std::string_view i3Str = hasFourth ?
            line.substr(thirdSep + 1) :
            "";

        auto readIndices = [](std::string_view v) -> obj::Face::Indices {
            // Eats the first element from the number and modifies the argument to contain
            // the rest
            auto eat = [](std::string_view& v, bool& done) {
                const size_t sep = v.find('/');

                std::string_view str = v.substr(0, sep);

                uint32_t res;
                std::from_chars_result convRes = std::from_chars(
                    str.data(), str.data() + str.size(),
                    res
                );
                if (convRes.ec != std::errc()) {
                    throw std::runtime_error("Error loading face");
                }

                if (sep == std::string_view::npos) {
                    done = true;
                }
                else {
                    done = false;
                    v = v.substr(sep + 1);
                }
                return res;
            };

            obj::Face::Indices res;
            bool isDone = false;
            res.vertex = eat(v, isDone);
            res.vertex -= 1; // 1-based indexing in Wavefront OBJ, 0-based in here

            if (!isDone) {
                res.uv = eat(v, isDone);
                *res.uv -= 1; // 1-based indexing in Wavefront OBJ, 0-based in here

                if (!isDone) {
                    res.normal = eat(v, isDone);
                    *res.normal -= 1; // 1-based indexing in Wavefront OBJ, 0-based in here
                }
            }

            return res;
        };

        obj::Face face;
        face.i0 = readIndices(i0Str);
        face.i1 = readIndices(i1Str);
        face.i2 = readIndices(i2Str);
        if (hasFourth) {
            face.i3 = readIndices(i3Str);
        }
        return face;
    }

} // namespace

namespace obj {

Model loadObjFile(const std::string& file) {
    std::ifstream f(file);
    if (!f.good()) {
        throw std::runtime_error("Could not find file " + file);
    }

    Model model;

    for (std::string line; std::getline(f, line);) {
        if (line.empty()) {
            continue;
        }

        if (line[0] == '#') {
            // We found a comment
            continue;
        }

        std::string tokenStr = line.substr(0, line.find(' '));
        Token token = tokenize(tokenStr);
        if (token == Token::Ignored) {
            continue;
        }
        if (token == Token::Unknown) {
            sgct::Log::Error("Unknown token: %s", tokenStr.data());
            continue;
        }
        std::string remainder = line.substr(line.find(' ') + 1);

        switch (token) {
            case Token::Vertex:
                model.positions.push_back(readPosition(remainder));
                break;
            case Token::Normal:
                model.normals.push_back(readNormal(remainder));
                break;
            case Token::UV:
                model.uvs.push_back(readUV(remainder));
                break;
            case Token::Face:
                model.faces.push_back(readFace(remainder));
                break;
            case Token::Ignored:
            case Token::Unknown:
                break;
        }
    }

    return model;
}

} // namespace obj
