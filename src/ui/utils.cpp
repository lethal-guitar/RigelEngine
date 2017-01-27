#include "utils.hpp"

#include "loader/resource_loader.hpp"


namespace rigel { namespace ui {

engine::OwningTexture fullScreenImageAsTexture(
  engine::Renderer* pRenderer,
  const loader::ResourceLoader& resources,
  const std::string& imageName
) {
  return engine::OwningTexture(
    pRenderer,
    resources.loadStandaloneFullscreenImage(imageName),
    false);
}

}}
