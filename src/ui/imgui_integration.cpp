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
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::ui::imgui_integration {

namespace {

bool shouldConsumeEvent(const SDL_Event& event) {
  const auto& io = ImGui::GetIO();

  switch (event.type) {
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

}


void init(SDL_Window* pWindow, void* pGlContext) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(pWindow, pGlContext);

#if defined(__APPLE__)
  ImGui_ImplOpenGL3_Init("#version 150");
#else
  // Dear ImGui can figure out the correct GLSL version by itself. This handles
  // GL ES as well as regular GL.
  ImGui_ImplOpenGL3_Init(nullptr);
#endif
}


void shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
}



bool handleEvent(const SDL_Event& event) {
  const auto handledEvent = ImGui_ImplSDL2_ProcessEvent(&event);

  return handledEvent && shouldConsumeEvent(event);
}


void beginFrame(SDL_Window* pWindow) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(pWindow);
  ImGui::NewFrame();
}


void endFrame() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}
