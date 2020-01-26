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
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace {
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

    std::tuple<GLuint, GLuint, uint32_t> createObjects(const std::vector<Vertex>& verts) {
        GLuint vao;
        GLuint vbo;
        uint32_t nVertices = static_cast<uint32_t>(verts.size());

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(Vertex) * verts.size(),
            verts.data(),
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

    std::tuple<GLuint, GLuint, uint32_t> loadObj(const std::string& filename,
                                                 bool printCornerVertices)
    {
        obj::Model obj = obj::loadObjFile(filename);

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

        if (printCornerVertices) {
            bool foundv00 = false;
            Vertex v00 = {
                0.f, 0.f, 0.f,
                0.f, 0.f, 0.f,
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()
            };
            bool foundv01 = false;
            Vertex v01 = {
                0.f, 0.f, 0.f,
                0.f, 0.f, 0.f,
                std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max()
            };
            bool foundv10 = false;
            Vertex v10 = {
                0.f, 0.f, 0.f,
                0.f, 0.f, 0.f,
                -std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()
            };
            bool foundv11 = false;
            Vertex v11 = {
                0.f, 0.f, 0.f,
                0.f, 0.f, 0.f,
                -std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max()
            };

            for (const Vertex& vertex : vertices) {
                if (vertex.u < v00.u && vertex.v < v00.v) {
                    v00 = vertex;
                    foundv00 = true;
                }
                if (vertex.u < v01.u && vertex.v > v01.v) {
                    v01 = vertex;
                    foundv01 = true;
                }
                if (vertex.u > v10.u && vertex.v < v10.v) {
                    v10 = vertex;
                    foundv10 = true;
                }
                if (vertex.u > v11.u && vertex.v > v11.v) {
                    v11 = vertex;
                    foundv11 = true;
                }
            }
            if (foundv00 && foundv01 && foundv10 && foundv11) {
                sgct::Log::Info("Vertex locations for %s", filename.c_str());
                sgct::Log::Info(
                    "LL (u=%f, v=%f): %f %f %f", v00.u, v00.v, v00.x, v00.y, v00.z
                );
                sgct::Log::Info(
                    "UL (u=%f, v=%f): %f %f %f", v01.u, v01.v, v01.x, v01.y, v01.z
                );
                sgct::Log::Info(
                    "LR (u=%f, v=%f): %f %f %f", v10.u, v10.v, v10.x, v10.y, v10.z
                );
                sgct::Log::Info(
                    "UR (u=%f, v=%f): %f %f %f", v11.u, v11.v, v11.x, v11.y, v11.z
                );
            }
            else {
                sgct::Log::Error(
                    "Error finding corner vertices %i %i %i %i",
                    foundv00, foundv01, foundv10, foundv11
                );
            }
        }

        return createObjects(vertices);
    }

    std::tuple<GLuint, GLuint, uint32_t> createCylinderGeometry(float r, float h) {
        constexpr const int Sections = 128;

        float sectorStep = glm::two_pi<float>() / (Sections - 1);

        std::vector<Vertex> vertices;
        for (int i = 0; i < Sections - 1; ++i) {
            float angle0 = i * sectorStep;
            float angle1 = (i + 1) * sectorStep;

            const float x0 = cos(angle0) * r;
            const float y0 = 0.f;
            const float z0 = sin(angle0) * r;

            const float x1 = cos(angle1) * r;
            const float y1 = h;
            const float z1 = sin(angle1) * r;


            // Lower left
            Vertex ll;
            ll.x = x0;
            ll.y = y0;
            ll.z = z0;
            ll.nx = -x0;
            ll.ny = -y0;
            ll.nz = -z0;
            ll.u = static_cast<float>(i) / static_cast<float>(Sections - 1);
            ll.v = 0.f;

            // Upper right
            Vertex ur;
            ur.x = x1;
            ur.y = y1;
            ur.z = z1;
            ur.nx = -x1;
            ur.ny = -y1;
            ur.nz = -z1;
            ur.u = static_cast<float>(i + 1) / static_cast<float>(Sections - 1);
            ur.v = 1.f;

            // Upper left
            Vertex ul;
            ul.x = x0;
            ul.y = y1;
            ul.z = z0;
            ul.nx = -x0;
            ul.ny = -y1;
            ul.nz = -z0;
            ul.u = static_cast<float>(i) / static_cast<float>(Sections - 1);
            ul.v = 1.f;

            // Lower right
            Vertex lr;
            lr.x = x1;
            lr.y = y0;
            lr.z = z1;
            lr.nx = -x1;
            lr.ny = -y0;
            lr.nz = -z1;
            lr.u = static_cast<float>(i + 1) / static_cast<float>(Sections - 1);
            lr.v = 0.f;

            vertices.push_back(ll);
            vertices.push_back(ur);
            vertices.push_back(ul);

            vertices.push_back(ll);
            vertices.push_back(lr);
            vertices.push_back(ur);
        }

        return createObjects(vertices);
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
        std::sort(res.begin(), res.end());
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

void Object::initializeFromModel(bool printCornerVertices) {
    sgct::Log::Info("Loading obj file %s", objFile.c_str());
    std::tuple<GLuint, GLuint, uint32_t> r = loadObj(objFile, printCornerVertices);
    vao = std::get<0>(r);
    vbo = std::get<1>(r);
    nVertices = std::get<2>(r);

#ifdef SGCT_HAS_SPOUT
    spout.senderName.resize(spoutName.size() + 1);
    std::fill(spout.senderName.begin(), spout.senderName.end(), '\0');
    std::copy(spoutName.begin(), spoutName.end(), spout.senderName.begin());

    spout.receiver = GetSpout();
#endif // SGCT_HAS_SPOUT
}

void Object::initializeFromCylinder(float radius, float height) {
    sgct::Log::Info("Loading cylinder");
    std::tuple<GLuint, GLuint, uint32_t> r = createCylinderGeometry(radius, height);
    vao = std::get<0>(r);
    vbo = std::get<1>(r);
    nVertices = std::get<2>(r);

#ifdef SGCT_HAS_SPOUT
    spout.senderName.resize(spoutName.size() + 1);
    std::fill(spout.senderName.begin(), spout.senderName.end(), '\0');
    std::copy(spoutName.begin(), spoutName.end(), spout.senderName.begin());

    spout.receiver = GetSpout();
#endif // SGCT_HAS_SPOUT
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
