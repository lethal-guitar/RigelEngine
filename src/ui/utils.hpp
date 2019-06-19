#pragma once

#include "renderer/texture.hpp"


namespace rigel::loader {
  class ResourceLoader;
}

namespace rigel::ui {

renderer::OwningTexture fullScreenImageAsTexture(
  renderer::Renderer* pRenderer,
  const loader::ResourceLoader& resources,
  const std::string& imageName);

}
