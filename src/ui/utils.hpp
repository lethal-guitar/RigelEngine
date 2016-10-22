#pragma once

#include <sdl_utils/texture.hpp>

namespace rigel { namespace loader {
  class ResourceBundle;
}}

namespace rigel { namespace ui {

sdl_utils::OwningTexture fullScreenImageAsTexture(
  SDL_Renderer* pRenderer,
  const loader::ResourceBundle& resources,
  const std::string& imageName);

}}
