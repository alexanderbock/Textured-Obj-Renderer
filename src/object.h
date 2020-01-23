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

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "imagecache.h"
#include <sgct/ogl_headers.h>
#ifdef SGCT_HAS_SPOUT
#include <SpoutLibrary.h>
#endif // SGCT_HAS_SPOUT
#include <filesystem>
#include <string>

struct Object {
    Object(std::string name, std::string objFile, std::string spoutName,
        std::string imageFolder);
    
    void initialize();
    void deinitialize();
    void bindTexture(bool useSpout);
    void unbindTexture(bool useSpout);

    GLuint vao = 0;
    GLuint vbo = 0;
    uint32_t nVertices = 0;

    const std::string name;
    const std::string objFile;
    const std::string spoutName;
    const std::vector<std::filesystem::path> imagePaths;

    ImageCache imageCache;

#ifdef SGCT_HAS_SPOUT
    struct Spout {
        SPOUTHANDLE receiver = nullptr;
        std::vector<char> senderName;
        unsigned int width = 0;
        unsigned int height = 0;
        bool isInitialized = false;
    };
    Spout spout;
#endif // SGCT_HAS_SPOUT
};

#endif // __OBJECT_H__
