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

#pragma once

#include "base/warnings.hpp"
#include "renderer/opengl.hpp"

RIGEL_DISABLE_WARNINGS
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
RIGEL_RESTORE_WARNINGS

#include <functional>
#include <initializer_list>
#include <string>
#include <unordered_map>


namespace rigel::renderer {

class GlHandleWrapper {
public:
  using DeleteFunc = std::function<void(GLuint)>;

  GlHandleWrapper() = default;

  template <typename DeleteFunc>
  explicit GlHandleWrapper(GLuint handle, DeleteFunc&& deleter)
    : mHandle(handle)
    , mDeleteFunc(std::forward<DeleteFunc>(deleter))
  {
  }

  GlHandleWrapper(const GlHandleWrapper&) = delete;
  GlHandleWrapper(GlHandleWrapper&& other)
    : mHandle(other.mHandle)
    , mDeleteFunc(other.mDeleteFunc)
  {
    other.mHandle = 0;
  }

  ~GlHandleWrapper() {
    mDeleteFunc(mHandle);
  }

  GlHandleWrapper& operator=(const GlHandleWrapper&) = delete;

  GLuint mHandle = 0;

private:
  DeleteFunc mDeleteFunc;
};


class Shader {
public:
  Shader(
    const char* preamble,
    const char* vertexSource,
    const char* fragmentSource,
    std::initializer_list<std::string> attributesToBind);

  void use();

  void setUniform(const std::string& name, const glm::mat4& matrix) {
    glUniformMatrix4fv(location(name), 1, GL_FALSE, glm::value_ptr(matrix));
  }

  void setUniform(const std::string& name, const glm::vec2& vec2) {
    glUniform2fv(location(name), 1, glm::value_ptr(vec2));
  }

  void setUniform(const std::string& name, const glm::vec3& vec3) {
    glUniform3fv(location(name), 1, glm::value_ptr(vec3));
  }

  void setUniform(const std::string& name, const glm::vec4& vec4) {
    glUniform4fv(location(name), 1, glm::value_ptr(vec4));
  }

  template <std::size_t N>
  void setUniform(
    const std::string& name,
    const std::array<glm::vec2, N>& values
  ) {
    glUniform3fv(location(name), N, glm::value_ptr(values.front()));
  }

  template <std::size_t N>
  void setUniform(
    const std::string& name,
    const std::array<glm::vec3, N>& values
  ) {
    glUniform3fv(location(name), N, glm::value_ptr(values.front()));
  }

  template <std::size_t N>
  void setUniform(
    const std::string& name,
    const std::array<glm::vec4, N>& values
  ) {
    glUniform3fv(location(name), N, glm::value_ptr(values.front()));
  }

  void setUniform(const std::string& name, const int value) {
    glUniform1i(location(name), value);
  }

  void setUniform(const std::string& name, const float value) {
    glUniform1f(location(name), value);
  }

  GLuint handle() {
    return mProgram.mHandle;
  }

private:
  GLint location(const std::string& name) const;

private:
  GlHandleWrapper mProgram;
  mutable std::unordered_map<std::string, GLint> mLocationCache;
};

}
