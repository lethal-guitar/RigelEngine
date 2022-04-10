/* Copyright (C) 2019, Nikolai Wuttke. All rights reserved.
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

#include "imgui_integration.hpp"

RIGEL_DISABLE_WARNINGS
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
RIGEL_RESTORE_WARNINGS

#include <algorithm>


namespace rigel::ui::imgui_integration
{

namespace
{

constexpr auto VERTICAL_4K_RES = 2160.0f;

constexpr auto IMGUI_DEFAULT_FONT_SIZE = 13;
constexpr auto INITIAL_UI_SCALE = 3.0f;

std::string gIniFilePath;

bool shouldConsumeEvent(const SDL_Event& event)
{
  const auto& io = ImGui::GetIO();

  switch (event.type)
  {
    case SDL_MOUSEWHEEL:
    case SDL_MOUSEBUTTONDOWN:
      return io.WantCaptureMouse;

    case SDL_TEXTINPUT:
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      return io.WantCaptureKeyboard;
  }

  return false;
}


void updateUiScale(const int, const int newHeight)
{
  // TODO: Implement proper DPI scaling

  // This is a very simple scaling that makes the UI look reasonably good on a
  // large 4k screen as well as on lower resolutions.  The idea is that 4k
  // (3840 x 2160) represents "full" size, and smaller vertical resolutions are
  // scaled down accordingly, i.e. half of 4k resolution (1080) would result in
  // a scale factor of 0.5.
  const auto scaleFactor =
    std::clamp(newHeight / VERTICAL_4K_RES, 1.0f / INITIAL_UI_SCALE, 1.0f);

  ImGui::GetIO().FontGlobalScale = scaleFactor;
  ImGui::GetStyle() = ImGuiStyle{};
  ImGui::GetStyle().ScaleAllSizes(scaleFactor * INITIAL_UI_SCALE);

  // AntiAliasedLinesUseTex requires using bilinear filtering, but we don't use
  // it (see our version of imgui_impl_opengl3.cpp).
  ImGui::GetStyle().AntiAliasedLinesUseTex = false;
}

} // namespace


void init(
  SDL_Window* pWindow,
  void* pGlContext,
  const std::optional<std::filesystem::path>& preferencesPath)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  {
    // We rasterize the font at a size that looks good at a 4k resolution, and
    // then scale it down for smaller screen sizes.
    ImFontConfig config;
    config.SizePixels = IMGUI_DEFAULT_FONT_SIZE * INITIAL_UI_SCALE;
    ImGui::GetIO().Fonts->AddFontDefault(&config);

    int width = 0;
    int height = 0;
    SDL_GL_GetDrawableSize(pWindow, &width, &height);
    updateUiScale(width, height);
  }

  ImGui_ImplSDL2_InitForOpenGL(pWindow, pGlContext);

  // Dear ImGui can figure out the correct GLSL version by itself. This handles
  // GL ES as well as regular GL.
  ImGui_ImplOpenGL3_Init(nullptr);

  if (preferencesPath)
  {
    const auto iniFilePath = *preferencesPath / "ImGui.ini";
    gIniFilePath = iniFilePath.u8string();
    ImGui::GetIO().IniFilename = gIniFilePath.c_str();
  }
  else
  {
    ImGui::GetIO().IniFilename = nullptr;
  }
}


void shutdown()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  gIniFilePath.clear();
}


bool handleEvent(const SDL_Event& event)
{
  const auto handledEvent = ImGui_ImplSDL2_ProcessEvent(&event);

  if (
    event.type == SDL_WINDOWEVENT &&
    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
  {
    updateUiScale(event.window.data1, event.window.data2);
  }

  return handledEvent && shouldConsumeEvent(event);
}


void beginFrame(SDL_Window* pWindow)
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(pWindow);
  ImGui::NewFrame();
}


void endFrame()
{
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace rigel::ui::imgui_integration
