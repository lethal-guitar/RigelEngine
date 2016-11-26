#include "utils.hpp"

#include "loader/resource_loader.hpp"


namespace rigel { namespace ui {

sdl_utils::OwningTexture fullScreenImageAsTexture(
  SDL_Renderer* pRenderer,
  const loader::ResourceLoader& resources,
  const std::string& imageName
) {
  return sdl_utils::OwningTexture(
    pRenderer,
    resources.loadStandaloneFullscreenImage(imageName),
    false);
}

}}
