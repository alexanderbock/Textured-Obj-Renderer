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

#include <glm/gtc/matrix_transform.hpp>
#include <sgct/sgct.h>
#include <string>
#include <vector>

using namespace sgct;

namespace {
    struct Buffers {
        GLuint vao;
        GLuint vbo;

        uint32_t nVertices;
    };

    Buffers loadObj(std::string filename) {
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

        Buffers res;
        res.nVertices = vertices.size();

        glGenVertexArrays(1, &res.vao);
        glBindVertexArray(res.vao);

        glGenBuffers(1, &res.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, res.vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(Vertex) * vertices.size(),
            vertices.data(),
            GL_STATIC_DRAW
        );


        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(6 * sizeof(float)));

        return res;
    }

    constexpr const char* vertexShader = R"(
#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

out vec3 tr_position;
out vec3 tr_normal;
out vec2 tr_uv;

uniform mat4 mvp;

void main() {
  gl_Position = mvp * vec4(in_position, 1.0);

  tr_position = in_position;
  tr_normal = in_normal;
  tr_uv = in_uv;
}
)";

    constexpr const char* fragmentShader = R"(
#version 330 core

in vec3 tr_position;
in vec3 tr_normal;
in vec2 tr_uv;

out vec4 color;

uniform vec3 lightPosition;

const vec3 AmbientColor = vec3(0.1, 0.1, 0.1);
const vec3 DiffuseColor = vec3(0.4, 0.4, 0.4);
const vec3 SpecularColor = vec3(1.0, 1.0, 1.0);

void main() {
  vec3 lightDir = normalize(lightPosition - tr_position);
  
  float lambertian = max(dot(lightDir, tr_normal), 0.0);
  float specular = 0.0;

  if (lambertian > 0.0) {
    vec3 viewDir = normalize(-tr_position);

    vec3 halfDir = normalize(lightDir + viewDir);
    float specAngle = max(dot(halfDir, tr_normal), 0.0);
    specular = pow(specAngle, 16.0);
  }

  color = vec4(vec3(tr_uv, 0.0) + lambertian * DiffuseColor + specular * SpecularColor, 1.0);
}
)";


    Buffers _wallA;
    Buffers _wallB;
    Buffers _wallC;
    Buffers _wallD;
    Buffers _wallR;

    glm::vec3 _eyePosition = glm::vec3(0.f, 0.f, 0.f);
    float _lookAtPhi = 0.f;
    glm::vec3 _upDirection = glm::vec3(0.f, 1.f, 0.f);

    glm::vec3 _lightPosition = glm::vec3(0.f, 0.f, 0.f);

} // namespace

void initGL(GLFWwindow*) {
    auto load = [](std::string filename) {
        Log::Info("Loading %s", filename.c_str());

        Buffers buffer = loadObj(filename);
        return buffer;
    };

    _wallA = load("../obj/wall_a_v20170909a.obj");
    _wallB = load("../obj/wall_b_v20170712a.obj");
    _wallC = load("../obj/wall_c_v20170712a.obj");
    _wallD = load("../obj/wall_d_v20170712a.obj");
    _wallR = load("../obj/wall_round_v20170909.obj");

    Log::Info("Finished loading");


    ShaderManager::instance().addShaderProgram("wall", vertexShader, fragmentShader);
}

void postSyncPreDraw() {

}

void draw(RenderData data) {
    glm::vec3 lookAtPosition = glm::vec3(
        std::cos(_lookAtPhi),
        _eyePosition.y,
        std::sin(_lookAtPhi)
    );

    glm::mat4 view = glm::lookAt(_eyePosition, lookAtPosition, _upDirection);
    glm::mat4 mvp = data.modelViewProjectionMatrix * view;


    const ShaderProgram& prog = ShaderManager::instance().shaderProgram("wall");
    prog.bind();

    glUniformMatrix4fv(
        glGetUniformLocation(prog.id(), "mvp"),
        1,
        GL_FALSE,
        glm::value_ptr(mvp)
    );

    glUniform3fv(
        glGetUniformLocation(prog.id(), "lightPosition"),
        1,
        glm::value_ptr(_lightPosition)
    );

    glBindVertexArray(_wallA.vao);
    glDrawArrays(GL_TRIANGLES, 0, _wallA.nVertices);

    glBindVertexArray(_wallB.vao);
    glDrawArrays(GL_TRIANGLES, 0, _wallB.nVertices);

    glBindVertexArray(_wallC.vao);
    glDrawArrays(GL_TRIANGLES, 0, _wallC.nVertices);

    glBindVertexArray(_wallD.vao);
    glDrawArrays(GL_TRIANGLES, 0, _wallD.nVertices);

    glBindVertexArray(_wallR.vao);
    glDrawArrays(GL_TRIANGLES, 0, _wallR.nVertices);

    prog.unbind();
}

void cleanUp() {
    glDeleteVertexArrays(1, &_wallA.vao);
    glDeleteBuffers(1, &_wallA.vbo);
    glDeleteVertexArrays(1, &_wallB.vao);
    glDeleteBuffers(1, &_wallB.vbo);
    glDeleteVertexArrays(1, &_wallC.vao);
    glDeleteBuffers(1, &_wallC.vbo);
    glDeleteVertexArrays(1, &_wallD.vao);
    glDeleteBuffers(1, &_wallD.vbo);
    glDeleteVertexArrays(1, &_wallR.vao);
    glDeleteBuffers(1, &_wallR.vbo);
}

void keyboardCallback(Key key, Modifier modifier, Action action, int) {
    if (action == Action::Release) {
        return;
    }

    switch (key) {
        case Key::Left:
            _lookAtPhi += 0.1f;
            break;
        case Key::Right:
            _lookAtPhi -= 0.1f;
            break;
        case Key::Up:
            _eyePosition.y += 1.f;
            break;
        case Key::Down:
            _eyePosition.y -= 1.f;
            break;
    }
}

void encode() {

}

void decode() {

}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initGL;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
    callbacks.cleanUp = cleanUp;
    callbacks.keyboard = keyboardCallback;
    callbacks.encode = encode;
    callbacks.decode = decode;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error & e) {
        Log::Error("%s", e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance().render();
    Engine::destroy();
    exit(EXIT_SUCCESS);


}
