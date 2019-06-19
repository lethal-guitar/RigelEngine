#include "utils.hpp"

#include "loader/resource_loader.hpp"


namespace rigel::ui {

renderer::OwningTexture fullScreenImageAsTexture(
  renderer::Renderer* pRenderer,
  const loader::ResourceLoader& resources,
  const std::string& imageName
) {
  return renderer::OwningTexture(
    pRenderer,
    resources.loadStandaloneFullscreenImage(imageName));
}

}
