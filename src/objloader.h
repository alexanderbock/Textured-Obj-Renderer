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

#ifndef __OBJLOADER_H__
#define __OBJLOADER_H__

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace obj {

struct Position {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

struct Normal {
    float nx = 0.f;
    float ny = 0.f;
    float nz = 0.f;
};

struct UV {
    float u = 0.f;
    float v = 0.f;
};

struct Face {
    struct Indices {
        uint32_t vertex = 0;
        std::optional<uint32_t> uv = std::nullopt;
        std::optional<uint32_t> normal = std::nullopt;
    };
    Indices i0;
    Indices i1;
    Indices i2;
    std::optional<Indices> i3;
};

struct Model {
    std::vector<Position> positions;
    std::vector<Normal> normals;
    std::vector<UV> uvs;

    std::vector<Face> faces;
};

Model loadObjFile(const std::string& file);


} // obj

#endif // __OBJLOADER_H__
