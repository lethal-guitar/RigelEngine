#pragma once

#include "engine/texture.hpp"


namespace rigel { namespace loader {
  class ResourceLoader;
}}

namespace rigel { namespace ui {

engine::OwningTexture fullScreenImageAsTexture(
  engine::Renderer* pRenderer,
  const loader::ResourceLoader& resources,
  const std::string& imageName);

}}
