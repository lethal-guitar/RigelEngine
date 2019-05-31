#pragma once

#include "renderer/texture.hpp"


namespace rigel { namespace loader {
  class ResourceLoader;
}}

namespace rigel { namespace ui {

renderer::OwningTexture fullScreenImageAsTexture(
  renderer::Renderer* pRenderer,
  const loader::ResourceLoader& resources,
  const std::string& imageName);

}}
