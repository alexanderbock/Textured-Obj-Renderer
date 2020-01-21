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

#include <fstream>
#include <utility>

namespace {
    std::string_view parseGroupName(std::string_view v) {
        const size_t opening = v.find('[');
        const size_t ending = v.find(']');
        return v.substr(opening, ending - opening);
    }

    std::string_view trim(std::string_view v) {
        // Remove spaces from the beginning of the string
        while (v[0] == ' ') {
            v = v.substr(1);
        }

        // Remove spaces from the end of the string
        while (v[v.size() - 1] == ' ') {
            v = v.substr(0, v.size() - 1);
        }

        return v;
    }

    std::pair<std::string_view, std::string_view> parseValue(std::string_view v) {
        const size_t equal = v.find('=');

        std::string_view begin = v.substr(0, equal);
        begin = trim(begin);

        std::string_view end = v.substr(equal + 1);
        end = trim(end);
        
        return std::pair(begin, end);
    }

} // namespace

Ini readIni(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.good()) {
        throw std::runtime_error("Could not find file " + filename);
    }

    Ini res;

    std::string currentGroup;
    for (std::string line; std::getline(f, line);) {
        if (line.empty()) {
            // Skip empty lines
            continue;
        }
        if (line[0] == '#') {
            // Treat # as a comment line
            continue;
        }

        if (line[0] == '[') {
            // We are in a group
            currentGroup = parseGroupName(line);

            // Ensure that the group exists in the map
            if (res.find(currentGroup) == res.end()) {
                res[currentGroup] = {};
            }
        }
        else {
            // We are finding a value
            if (currentGroup.empty()) {
                throw std::runtime_error("Found a value outside a group");
            }

            std::pair<std::string_view, std::string_view> value = parseValue(line);
            res[currentGroup][std::string(value.first)] = value.second;
        }
    }

    return res;
}