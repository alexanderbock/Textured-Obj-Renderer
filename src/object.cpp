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

#include "object.h"

#include "objloader.h"
#include <sgct/log.h>

namespace {
    std::tuple<GLuint, GLuint, uint32_t> loadObj(const std::string& filename) {
        obj::Model obj = obj::loadObjFile(filename);

        struct Vertex {
            float x = 0.f;
            float y = 0.f;
            float z = 0.f;

            float nx = 0.f;
            float ny = 0.f;
            float nz = 1.f;

            float u = 0.f;
            float v = 0.f;
        };

        std::vector<Vertex> vertices;

        for (const obj::Face& face : obj.faces) {
            auto makeVertex = [&obj](obj::Face::Indices indices) -> Vertex {
                Vertex res;

                res.x = obj.positions[indices.vertex].x;
                res.y = obj.positions[indices.vertex].y;
                res.z = obj.positions[indices.vertex].z;

                if (indices.normal.has_value()) {
                    res.nx = obj.normals[*indices.normal].nx;
                    res.ny = obj.normals[*indices.normal].ny;
                    res.nz = obj.normals[*indices.normal].nz;
                }

                if (indices.uv.has_value()) {
                    res.u = obj.uvs[*indices.uv].u;
                    res.v = obj.uvs[*indices.uv].v;
                }

                return res;
            };


            vertices.push_back(makeVertex(face.i0));
            vertices.push_back(makeVertex(face.i1));
            vertices.push_back(makeVertex(face.i2));

            if (face.i3.has_value()) {
                vertices.push_back(makeVertex(face.i0));
                vertices.push_back(makeVertex(face.i2));
                vertices.push_back(makeVertex(*face.i3));
            }
        }

        GLuint vao;
        GLuint vbo;
        uint32_t nVertices = static_cast<uint32_t>(vertices.size());

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(Vertex) * vertices.size(),
            vertices.data(),
            GL_STATIC_DRAW
        );


        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            nullptr
        );

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(3 * sizeof(float))
        );

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(
            2,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(6 * sizeof(float))
        );

        return { vao, vbo, nVertices };
    }

    std::vector<std::filesystem::path> loadImagePaths(std::string imageFolder) {
        if (imageFolder.empty()) {
            return std::vector<std::filesystem::path>();
        }
        namespace fs = std::filesystem;
        std::vector<fs::path> res;
        for (const fs::directory_entry& entry : fs::directory_iterator(imageFolder)) {
            res.push_back(entry);
        }
        return res;
    }
} // namespace


Object::Object(std::string name_, std::string objFile_, std::string spoutName_,
               std::string imageFolder_)
    : name(std::move(name_))
    , objFile(std::move(objFile_))
    , spoutName(std::move(spoutName_))
    , imagePaths(loadImagePaths(imageFolder_))
    , imageCache(imagePaths)
{}

void Object::initialize() {
    sgct::Log::Info("Loading obj file %s", objFile.c_str());
    std::tuple<GLuint, GLuint, uint32_t> r = loadObj(objFile);
    vao = std::get<0>(r);
    vbo = std::get<1>(r);
    nVertices = std::get<2>(r);

    spout.senderName.resize(spoutName.size() + 1);
    std::fill(spout.senderName.begin(), spout.senderName.end(), '\0');
    std::copy(spoutName.begin(), spoutName.end(), spout.senderName.begin());

    spout.receiver = GetSpout();
}

void Object::deinitialize() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

#ifdef SGCT_HAS_SPOUT
    if (spout.receiver) {
        spout.receiver->ReleaseReceiver();
        spout.receiver->Release();
    }
#endif // SGCT_HAS_SPOUT
}

void Object::bindTexture(bool useSpout) {
    if (useSpout) {
#ifdef SGCT_HAS_SPOUT
        spout.isInitialized = spout.receiver->CreateReceiver(
            spout.senderName.data(),
            spout.width,
            spout.height
        );
        if (spout.isInitialized) {
            const bool success = spout.receiver->ReceiveTexture(
                spout.senderName.data(),
                spout.width,
                spout.height
            );
            if (success) {
                spout.receiver->BindSharedTexture();
            }
        }
#endif // SGCT_HAS_SPOUT
    }
    else {
        glBindTexture(GL_TEXTURE_2D, imageCache.texture());
    }
}

void Object::unbindTexture(bool useSpout) {
    if (useSpout) {
#ifdef SGCT_HAS_SPOUT
        if (spout.isInitialized) {
            spout.receiver->UnBindSharedTexture();
        }
#endif // SGCT_HAS_SPOUT
    }
    else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
