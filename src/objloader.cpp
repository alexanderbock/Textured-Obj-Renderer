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
#include <charconv>
#include <fstream>
#include <functional>

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


    obj::Position readPosition(std::string_view line) {
        const size_t firstSeparator = line.find(' ');
        const size_t secondSeparator = line.find(' ', firstSeparator + 1);
        std::string_view xStr = line.substr(0, firstSeparator);
        std::string_view yStr = line.substr(
            firstSeparator + 1,
            secondSeparator - firstSeparator - 1
        );
        std::string_view zStr = line.substr(secondSeparator + 1);

        obj::Position res;

        std::from_chars_result xRes = std::from_chars(
            xStr.data(), xStr.data() + xStr.size(),
            res.x
        );

        std::from_chars_result yRes = std::from_chars(
            yStr.data(), yStr.data() + yStr.size(),
            res.y
        );

        std::from_chars_result zRes = std::from_chars(
            zStr.data(), zStr.data() + zStr.size(),
            res.z
        );

        if (xRes.ec != std::errc() || yRes.ec != std::errc() ||
            zRes.ec != std::errc())
        {
            throw std::runtime_error("Error loading line: " + std::string(line));
        }

        return res;
    }

    obj::Normal readNormal(std::string_view line) {
        const size_t firstSeparator = line.find(' ');
        const size_t secondSeparator = line.find(' ', firstSeparator + 1);
        std::string_view xStr = line.substr(0, firstSeparator);
        std::string_view yStr = line.substr(
            firstSeparator + 1,
            secondSeparator - firstSeparator - 1
        );
        std::string_view zStr = line.substr(secondSeparator + 1);

        obj::Normal res;

        std::from_chars_result xRes = std::from_chars(
            xStr.data(), xStr.data() + xStr.size(),
            res.nx
        );

        std::from_chars_result yRes = std::from_chars(
            yStr.data(), yStr.data() + yStr.size(),
            res.ny
        );

        std::from_chars_result zRes = std::from_chars(
            zStr.data(), zStr.data() + zStr.size(),
            res.nz
        );

        if (xRes.ec != std::errc() || yRes.ec != std::errc() ||
            zRes.ec != std::errc())
        {
            throw std::runtime_error("Error loading line: " + std::string(line));
        }

        return res;
    }

    obj::UV readUV(std::string_view line) {
        const size_t separator = line.find(' ');

        std::string_view uStr = line.substr(0, separator);
        std::string_view vStr = line.substr(separator + 1);

        obj::UV res;

        std::from_chars_result uRes = std::from_chars(
            uStr.data(), uStr.data() + uStr.size(),
            res.u
        );

        std::from_chars_result vRes = std::from_chars(
            vStr.data(), vStr.data() + vStr.size(),
            res.v
        );

        if (uRes.ec != std::errc() || vRes.ec != std::errc()) {
            throw std::runtime_error("Error loading line: " + std::string(line));
        }
        
        return res;
    }

    obj::Face readFace(std::string_view line) {
        const size_t firstSep = line.find(' ');
        const size_t secondSep = line.find(' ', firstSep + 1);
        const bool hasThird = secondSep != std::string_view::npos;
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
            std::function<uint32_t(std::string_view & v, bool& done)> eat =
                [&eat](std::string_view& v, bool& done)
            {
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



            //const size_t first = v.find('/');
            //const size_t second = v.find('/', first + 1);

            //std::string_view vert = v.substr(0, first);
            //std::string_view uv = v.substr(first + 1, second - first - 1);
            //std::string_view norm = v.substr(second + 1);

            //std::from_chars_result vertRes = std::from_chars(
            //    vert.data(), vert.data() + vert.size(),
            //    res.vertex
            //);
            //res.vertex -= 1;  // 1-based indexing in Waveform OBJ, 0-based in here

            //std::from_chars_result uvRes = std::from_chars(
            //    uv.data(), uv.data() + uv.size(),
            //    res.uv
            //);
            //res.uv -= 1;  // 1-based indexing in Waveform OBJ, 0-based in here

            //std::from_chars_result normRes = std::from_chars(
            //    norm.data(), norm.data() + norm.size(),
            //    res.normal
            //);
            //res.normal -= 1;  // 1-based indexing in Waveform OBJ, 0-based in here

            //if (vertRes.ec != std::errc() || uvRes.ec != std::errc() ||
            //    normRes.ec != std::errc())
            //{
            //    throw std::runtime_error("Error loading line: " + std::string(v));
            //}

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
        }
    }

    return model;
}

} // namespace obj
