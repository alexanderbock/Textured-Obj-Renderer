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

#include "inireader.h"
#include "object.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <sgct/sgct.h>
#include <glfw/glfw3.h>
#include <filesystem>
#include <string>
#include <vector>

using namespace sgct;

namespace {
    constexpr const float Sensitivity = 750.f;

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
  
  float l = max(dot(lightDir, tr_normal), 0.0);
  float spec = 0.0;

  if (l > 0.0) {
    vec3 viewDir = normalize(-tr_position);

    vec3 halfDir = normalize(lightDir + viewDir);
    float specAngle = max(dot(halfDir, tr_normal), 0.0);
    spec = pow(specAngle, 16.0);
  }

  color = vec4(vec3(tr_uv, 0.0) + l * DiffuseColor + spec * SpecularColor, 1.0);
}
)";

    std::vector<Object> _objects;

    bool _leftButtonDown = false;
    bool _rightButtonDown = false;

    glm::vec3 _eyePosition = glm::vec3(0.f, 0.f, 0.f);
    double _lookAtPhi = 0.0;
    double _lookAtTheta = 0.0;
    glm::vec3 _lightPosition = glm::vec3(0.f, 0.f, 0.f);


    struct SyncData {
        float eyePosX;
        float eyePosY;
        float eyePosZ;

        double lookAtPhi;
        double lookAtTheta;

        float lightPosX;
        float lightPosY;
        float lightPosZ;
    };
    SharedObject<SyncData> _syncData;

} // namespace

void initGL(GLFWwindow*) {
    std::for_each(_objects.begin(), _objects.end(), std::mem_fn(&Object::upload));
    Log::Info("Finished loading");

    ShaderManager::instance().addShaderProgram("wall", vertexShader, fragmentShader);
}

void postSyncPreDraw() {
}

void draw(RenderData data) {
    glm::mat4 translation = glm::translate(glm::mat4(1.f), _eyePosition);

    glm::quat phiRotation = glm::angleAxis(
        static_cast<float>(_lookAtPhi),
        glm::vec3(0.f, 1.f, 0.f)
    );
    glm::quat thetaRotation = glm::angleAxis(
        static_cast<float>(_lookAtTheta),
        glm::vec3(1.f, 0.f, 0.f)
    );
    glm::quat view = thetaRotation * phiRotation;
    glm::mat4 mvp = data.modelViewProjectionMatrix * glm::mat4_cast(view) * translation;

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

    for (const Object& obj : _objects) {
        glBindVertexArray(obj.vao);
        glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);
    }

    prog.unbind();
}

void cleanUp() {
    for (const Object& obj : _objects) {
        glDeleteVertexArrays(1, &obj.vao);
        glDeleteBuffers(1, &obj.vbo);
    }
    _objects.clear();
}

void keyboardCallback(Key key, Modifier modifier, Action action, int) {
    if (action == Action::Release) {
        return;
    }

    switch (key) {
        case Key::W:
            _eyePosition += glm::vec3(0.1f, 0.f, 0.f);
            break;
        case Key::S:
            _eyePosition -= glm::vec3(0.1f, 0.f, 0.f);
            break;
        case Key::A:
            _eyePosition += glm::vec3(0.f, 0.f, 0.1f);
            break;
        case Key::D:
            _eyePosition -= glm::vec3(0.f, 0.f, 0.1f);
            break;
    }
}

void mousePos(double x, double y) {
    int width, height;
    GLFWwindow* w = glfwGetCurrentContext();
    glfwGetWindowSize(w, &width, &height);

    const double dx = (x - width / 2) / Sensitivity;
    const double dy = (y - height / 2) / Sensitivity;

    if (_leftButtonDown) {
        _lookAtPhi += dx;
        _lookAtTheta = std::clamp(
            _lookAtTheta + dy,
            -glm::half_pi<double>(),
            glm::half_pi<double>()
        );
    }

    if (_rightButtonDown) {
        _eyePosition.y += dy;
    }

    if (_leftButtonDown || _rightButtonDown) {
        glfwSetCursorPos(w, width / 2, height / 2);
    }
}

void mouseButton(MouseButton button, Modifier modifier, Action action) {
    if (button == MouseButton::Button1) {
        _leftButtonDown = action == Action::Press;
    }

    if (button == MouseButton::Button2) {
        _rightButtonDown = action == Action::Press;
    }

    GLFWwindow* w = glfwGetCurrentContext();
    glfwSetInputMode(
        w,
        GLFW_CURSOR,
        (_leftButtonDown || _rightButtonDown) ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL
    );
    int width, height;
    glfwGetWindowSize(w, &width, &height);

    if (_leftButtonDown || _rightButtonDown) {
        glfwSetCursorPos(w, width / 2, height / 2);
    }
}

void encode() {
    SyncData data;
    data.eyePosX = _eyePosition.x;
    data.eyePosY = _eyePosition.y;
    data.eyePosZ = _eyePosition.z;

    data.lookAtPhi = _lookAtPhi;
    data.lookAtTheta = _lookAtTheta;

    data.lightPosX = _lightPosition.x;
    data.lightPosY = _lightPosition.y;
    data.lightPosZ = _lightPosition.z;

    _syncData.setValue(data);
    sgct::SharedData::instance().writeObj(_syncData);
}

void decode() {
    sgct::SharedData::instance().readObj(_syncData);
    const SyncData& data = _syncData.value();

    _eyePosition = glm::vec3(data.eyePosX, data.eyePosY, data.eyePosZ);
    _lookAtPhi = data.lookAtPhi;
    _lookAtTheta = data.lookAtTheta;
    _lightPosition = glm::vec3(data.lightPosX, data.lightPosY, data.lightPosZ);
}

int main(int argc, char** argv) {
    std::filesystem::path iniPath = "config.ini";
    while (!std::filesystem::exists(iniPath) && iniPath != iniPath.root_path() ) {
        iniPath = std::filesystem::absolute(".." / iniPath);
    }
    if (iniPath == iniPath.root_path()) {
        throw std::runtime_error("Could not find 'config.ini'");
    }
    
    if (!iniPath.parent_path().empty()) {
        std::filesystem::current_path(iniPath.parent_path());
    }

    Log::Info("Loading ini file %s", iniPath.string().c_str());
    Ini ini = readIni(iniPath.string());

    std::map<std::string, std::string> models = ini["Models"];
    std::map<std::string, std::string> imagePaths = ini["Image"];

    for (const std::pair<const std::string, std::string>& p : models) {
        const std::string& modelPath = p.second;
        const std::string& imagePath = imagePaths[p.first];

        _objects.emplace_back(modelPath, imagePath);
    }

    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initGL;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
    callbacks.cleanUp = cleanUp;
    callbacks.keyboard = keyboardCallback;
    callbacks.mousePos = mousePos;
    callbacks.mouseButton = mouseButton;
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
