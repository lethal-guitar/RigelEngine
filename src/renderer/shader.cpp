/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "shader.hpp"

#include <memory>
#include <stdexcept>
#include <string>


namespace rigel { namespace renderer {

namespace {

GlHandleWrapper compileShader(const std::string& source, GLenum type) {
  auto shader = GlHandleWrapper{glCreateShader(type), glDeleteShader};
  const auto sourcePtr = source.c_str();
  glShaderSource(shader.mHandle, 1, &sourcePtr, nullptr);
  glCompileShader(shader.mHandle);

  GLint compileStatus = 0;
  glGetShaderiv(shader.mHandle, GL_COMPILE_STATUS, &compileStatus);

  if (!compileStatus) {
    GLint infoLogSize = 0;
    glGetShaderiv(shader.mHandle, GL_INFO_LOG_LENGTH, &infoLogSize);

    if (infoLogSize > 0) {
      std::unique_ptr<char[]> infoLogBuffer(new char[infoLogSize]);
      glGetShaderInfoLog(
        shader.mHandle, infoLogSize, nullptr, infoLogBuffer.get());

      throw std::runtime_error(
        std::string{"Shader compilation failed:\n\n"} + infoLogBuffer.get());
    } else {
      throw std::runtime_error(
        "Shader compilation failed, but could not get info log");
    }
  }

  return shader;
}

}


Shader::Shader(
  const char* preamble,
  const char* vertexSource,
  const char* fragmentSource,
  std::initializer_list<std::string> attributesToBind)
  : mProgram(glCreateProgram(), glDeleteProgram)
{
  auto vertexShader = compileShader(
    std::string{preamble} + vertexSource, GL_VERTEX_SHADER);
  auto fragmentShader = compileShader(
    std::string{preamble} + fragmentSource, GL_FRAGMENT_SHADER);

  glAttachShader(mProgram.mHandle, vertexShader.mHandle);
  glAttachShader(mProgram.mHandle, fragmentShader.mHandle);

  int index = 0;
  for (const auto& attributeName : attributesToBind) {
    glBindAttribLocation(mProgram.mHandle, index, attributeName.c_str());
    ++index;
  }

  glLinkProgram(mProgram.mHandle);

  GLint linkStatus = 0;
  glGetProgramiv(mProgram.mHandle, GL_LINK_STATUS, &linkStatus);
  if (!linkStatus) {
    GLint infoLogSize = 0;
    glGetProgramiv(mProgram.mHandle, GL_INFO_LOG_LENGTH, &infoLogSize);

    if (infoLogSize > 0) {
      std::unique_ptr<char[]> infoLogBuffer(new char[infoLogSize]);
      glGetProgramInfoLog(
        mProgram.mHandle, infoLogSize, nullptr, infoLogBuffer.get());

      throw std::runtime_error(
        std::string{"Shader program linking failed:\n\n"} +
        infoLogBuffer.get()
      );
    } else {
      throw std::runtime_error(
        "Shader program linking failed, but could not get info log");
    }
  }
}


void Shader::use() {
  glUseProgram(mProgram.mHandle);
}


GLint Shader::location(const std::string& name) const {
  auto it = mLocationCache.find(name);
  if (it == mLocationCache.end()) {
    const auto location = glGetUniformLocation(mProgram.mHandle, name.c_str());
    std::tie(it, std::ignore) = mLocationCache.emplace(name, location);
  }

  return it->second;
}

}}
