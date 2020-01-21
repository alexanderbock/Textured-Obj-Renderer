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

#include "imagecache.h"

#include <sgct/log.h>
#include <sgct/texturemanager.h>

ImageCache::ImageCache(std::vector<std::filesystem::path> paths)
    : _paths(std::move(paths))
{
    //std::fill(_imageCache.begin(), _imageCache.end(), 0);
}

void ImageCache::setCurrentImage(uint32_t currentImage) {
    const bool cacheDirty = currentImage != _currentImage;
    if (currentImage == _currentImage) {
        return;
    }
    if (currentImage >= _paths.size()) {
        return;
    }

    _currentImage = currentImage;

    if (_texture > 0) {
        sgct::TextureManager::instance().removeTexture(_texture);
    }
    std::string path = _paths[_currentImage].string();
    sgct::Log::Debug("Loading image %s", path.c_str());
    _texture = sgct::TextureManager::instance().loadTexture(path, true);
}

GLuint ImageCache::texture() const {
    return _texture;
}

std::string ImageCache::loadedImage() const {
    if (_currentImage < _paths.size()) {
        return _paths[_currentImage].string();
    }
    else {
        return "";
    }
}
