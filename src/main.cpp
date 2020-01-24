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
#include <charconv>
#include <filesystem>
#include <string>
#include <vector>

using namespace sgct;

namespace {
    constexpr const float Sensitivity = 750.f;

    constexpr const char* VertexShader = R"(
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

    constexpr const char* FragmentShader = R"(
#version 330 core

in vec3 tr_position;
in vec3 tr_normal;
in vec2 tr_uv;

out vec4 color;

uniform sampler2D tex;
uniform int flipTex;

void main() {
  vec2 texCoords = tr_uv;
  if (flipTex != 0) {
    texCoords.y = 1.0 - texCoords.y;
  }
  color = texture(tex, texCoords);
}
)";

    // Synchronized values
    glm::vec3 eyePosition = glm::vec3(0.f, 0.f, 0.f);
    double lookAtPhi = 0.0;
    double lookAtTheta = 0.0;

    bool useSpoutTextures = false;
    uint32_t currentImage = 0;
    bool showHelp = false;

    // State value
    std::vector<Object> objects;

    bool leftButtonDown = false;
    bool rightButtonDown = false;
    bool playingImages = false;
    bool printCornerVertices = false;

} // namespace

void initGL(GLFWwindow*) {
    for (Object& obj : objects) {
        obj.initialize(printCornerVertices);
        obj.imageCache.setCurrentImage(0);
    }
    Log::Info("Finished loading");

    ShaderManager::instance().addShaderProgram("wall", VertexShader, FragmentShader);
}

void preSync() {
    if (playingImages) {
        currentImage += 1;
    }
}

void postSyncPreDraw() {
    for (Object& obj : objects) {
        obj.imageCache.setCurrentImage(currentImage);
    }
}

void draw(const RenderData& data) {
    glEnable(GL_CULL_FACE);

    glm::mat4 translation = glm::translate(glm::mat4(1.f), eyePosition);

    glm::quat phiRotation = glm::angleAxis(
        static_cast<float>(lookAtPhi),
        glm::vec3(0.f, 1.f, 0.f)
    );
    glm::quat thetaRotation = glm::angleAxis(
        static_cast<float>(lookAtTheta),
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

    glUniform1i(glGetUniformLocation(prog.id(), "tex"), 0);

    glUniform1i(glGetUniformLocation(prog.id(), "flipTex"), useSpoutTextures ? 1 : 0);
    
    glActiveTexture(GL_TEXTURE0);
    for (Object& obj : objects) {
        obj.bindTexture(useSpoutTextures);

        glBindVertexArray(obj.vao);
        glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);

        obj.unbindTexture(useSpoutTextures);
    }

    prog.unbind();

    glDisable(GL_CULL_FACE);
}

void draw2D(const RenderData& data) {
    if (!showHelp) {
        return;
    }

    float w = static_cast<float>(data.window.resolution().x) * data.viewport.size().x;
    text::Font* f1 = text::FontManager::instance().font("SGCTFont", 14);

    if (Engine::instance().isMaster()) {
        text::print(
            data.window,
            data.viewport,
            *f1,
            text::Alignment::TopLeft,
            (5.f * w) / 7.f,
            250.f,
            glm::vec4(0.8f, 0.8f, 0.f, 1.f),
            "Help\nWSAD: Move camera\nSpace: Play/stop images\nUp/Down: Advance images\n"
            "1: Back to first image"
        );
    }

    float h = 25.f;
    for (const Object& obj : objects) {
        if (useSpoutTextures) {
#ifdef SGCT_HAS_SPOUT
            text::print(
                data.window,
                data.viewport,
                *f1,
                text::Alignment::TopLeft,
                25.f,
                h,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "%s: %s",
                obj.name.c_str(),
                obj.spoutName.c_str()
            );
#else // SGCT_HAS_SPOUT
            text::print(
                data.window,
                data.viewport,
                *f1,
                text::Alignment::TopLeft,
                25.f,
                h,
                glm::vec4(0.8f, 0.2f, 0.2f, 1.f),
                "Compiled with Spout support"
            );
            break;
#endif // SGCT_HAS_SPOUT
        }
        else {
            text::print(
                data.window,
                data.viewport,
                *f1,
                text::Alignment::TopLeft,
                25.f,
                h,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "%s: %s (%i)",
                obj.name.c_str(),
                obj.imageCache.loadedImage().c_str(),
                obj.imageCache.texture()
            );
        }
        h += 25.f;
    }

    if (useSpoutTextures) {
        text::print(
            data.window,
            data.viewport,
            *f1,
            text::Alignment::TopLeft,
            25.f,
            h,
            glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
            "Spout"
        );
    }
    else {
        text::print(
            data.window,
            data.viewport,
            *f1,
            text::Alignment::TopLeft,
            25.f,
            h,
            glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
            "Images // Current image: %i", currentImage
        );
    }
}

void cleanup() {
    for (Object& obj : objects) {
        obj.deinitialize();
    }
    objects.clear();
}

void keyboard(Key key, Modifier, Action action, int) {
    if (action == Action::Release) {
        return;
    }

    switch (key) {
        case Key::W:
            eyePosition += glm::vec3(0.1f, 0.f, 0.f);
            break;
        case Key::S:
            eyePosition -= glm::vec3(0.1f, 0.f, 0.f);
            break;
        case Key::A:
            eyePosition += glm::vec3(0.f, 0.f, 0.1f);
            break;
        case Key::D:
            eyePosition -= glm::vec3(0.f, 0.f, 0.1f);
            break;
        case Key::Space:
            playingImages = !playingImages;
            break;
        case Key::Up:
            currentImage += 1;
            break;
        case Key::Down:
            currentImage = std::max(currentImage - 1, 0u);
            break;
        case Key::F1:
            showHelp = !showHelp;
            break;
        case Key::Key1:
            currentImage = 0;
            playingImages = false;
            useSpoutTextures = false;
            break;
        case Key::Key2:
            currentImage = 0;
            playingImages = false;
            useSpoutTextures = true;
            break;
    }
}

void mousePos(double x, double y) {
    int width, height;
    GLFWwindow* w = glfwGetCurrentContext();
    glfwGetWindowSize(w, &width, &height);

    const double dx = (x - width / 2) / Sensitivity;
    const double dy = (y - height / 2) / Sensitivity;

    if (leftButtonDown) {
        lookAtPhi += dx;
        lookAtTheta = std::clamp(
            lookAtTheta + dy,
            -glm::half_pi<double>(),
            glm::half_pi<double>()
        );
    }

    if (rightButtonDown) {
        eyePosition.y += static_cast<float>(dy);
    }

    if (leftButtonDown || rightButtonDown) {
        glfwSetCursorPos(w, width / 2, height / 2);
    }
}

void mouseButton(MouseButton button, Modifier, Action action) {
    if (button == MouseButton::Button1) {
        leftButtonDown = action == Action::Press;
    }

    if (button == MouseButton::Button2) {
        rightButtonDown = action == Action::Press;
    }

    GLFWwindow* w = glfwGetCurrentContext();
    glfwSetInputMode(
        w,
        GLFW_CURSOR,
        (leftButtonDown || rightButtonDown) ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL
    );
    int width, height;
    glfwGetWindowSize(w, &width, &height);

    if (leftButtonDown || rightButtonDown) {
        glfwSetCursorPos(w, width / 2, height / 2);
    }
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, eyePosition.x);
    serializeObject(data, eyePosition.y);
    serializeObject(data, eyePosition.z);

    serializeObject(data, lookAtPhi);
    serializeObject(data, lookAtTheta);

    serializeObject(data, currentImage);
    serializeObject(data, showHelp);
    serializeObject(data, useSpoutTextures);
    return data;
}

void decode(const std::vector<std::byte>& data, unsigned int pos) {
    deserializeObject(data, pos, eyePosition.x);
    deserializeObject(data, pos, eyePosition.y);
    deserializeObject(data, pos, eyePosition.z);

    deserializeObject(data, pos, lookAtPhi);
    deserializeObject(data, pos, lookAtTheta);

    deserializeObject(data, pos, currentImage);
    deserializeObject(data, pos, showHelp);
    deserializeObject(data, pos, useSpoutTextures);
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
    std::map<std::string, std::string> spoutNames = ini["Spout"];
    
    std::map<std::string, std::string> misc = ini["Misc"];
    std::string height = misc["CameraHeight"];
    std::from_chars(height.data(), height.data() + height.size(), eyePosition.y);
    eyePosition.y = -eyePosition.y;

    const std::string outputCornersStr = misc["OutputCornerVertices"];
    printCornerVertices = outputCornersStr == "true";

    for (const std::pair<const std::string, std::string>& p : models) {
        std::string modelPath = p.second;
        std::string imagePath =
            imagePaths.find(p.first) != imagePaths.end() ? imagePaths[p.first] : "";
        std::string spoutName =
            spoutNames.find(p.first) != spoutNames.end() ? spoutNames[p.first] : "";

        Object obj(
            p.first,
            std::move(modelPath),
            std::move(spoutName),
            std::move(imagePath)
        );
        objects.push_back(std::move(obj));
    }



    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    Engine::Callbacks callbacks;
    callbacks.initOpenGL = initGL;
    callbacks.preSync = preSync;
    callbacks.encode = encode;
    callbacks.decode = decode;
    callbacks.postSyncPreDraw = postSyncPreDraw;
    callbacks.draw = draw;
    callbacks.draw2D = draw2D;
    callbacks.cleanup = cleanup;
    callbacks.keyboard = keyboard;
    callbacks.mousePos = mousePos;
    callbacks.mouseButton = mouseButton;

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
