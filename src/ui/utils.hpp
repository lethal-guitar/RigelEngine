#pragma once

#include "sdl_utils/texture.hpp"


namespace rigel { namespace loader {
  class ResourceLoader;
}}

namespace rigel { namespace ui {

sdl_utils::OwningTexture fullScreenImageAsTexture(
  SDL_Renderer* pRenderer,
  const loader::ResourceLoader& resources,
  const std::string& imageName);

}}
