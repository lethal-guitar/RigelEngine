/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
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

#include "game_path_browser.hpp"

#include "base/math_tools.hpp"
#include "renderer/opengl.hpp"
#include "ui/imgui_integration.hpp"

RIGEL_DISABLE_WARNINGS
#include <imfilebrowser.h>
#include <imgui.h>
RIGEL_RESTORE_WARNINGS


namespace rigel::ui
{

std::filesystem::path runFolderBrowser(SDL_Window* pWindow)
{
  auto folderPath = std::filesystem::path();
  auto folderBrowser =
    ImGui::FileBrowser{ImGuiFileBrowserFlags_SelectDirectory};

  // TODO: There is some code duplication with the game path browser in the
  // options menu for setting the size and title, but until we've decided if we
  // will merge those two into one by showing the options menu at first launch
  // or not, we'll leave it like this. Should we decide against showing the
  // options menu at first launch, we should extract some constants and helper
  // functions to avoid this duplication.
  folderBrowser.SetTitle("Choose Duke Nukem II installation");

  {
    int windowWidth = 0;
    int windowHeight = 0;
    SDL_GetWindowSize(pWindow, &windowWidth, &windowHeight);
    folderBrowser.SetWindowSize(
      base::round(windowWidth * 0.64f), base::round(windowHeight * 0.64f));
  }

  folderBrowser.Open();

  do
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      ui::imgui_integration::handleEvent(event);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    ui::imgui_integration::beginFrame(pWindow);

    folderBrowser.Display();
    if (folderBrowser.HasSelected())
    {
      folderPath = folderBrowser.GetSelected();
      folderBrowser.Close();
    }

    ui::imgui_integration::endFrame();
    SDL_GL_SwapWindow(pWindow);
  } while (folderBrowser.IsOpened());

  return folderPath;
}


void showErrorMessage(SDL_Window* pWindow, const std::string& error)
{
  SDL_Event event;
  auto boxIsVisible = true;
  auto firstTime = true;

  while (boxIsVisible)
  {
    while (SDL_PollEvent(&event))
    {
      ui::imgui_integration::handleEvent(event);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    ui::imgui_integration::beginFrame(pWindow);

    if (firstTime)
    {
      firstTime = false;
      ImGui::OpenPopup("Error!");
    }

    if (ImGui::BeginPopupModal("Error!", &boxIsVisible, 0))
    {
      ImGui::Text("%s", error.c_str());

      if (ImGui::Button("Ok"))
      {
        boxIsVisible = false;
      }

      ImGui::EndPopup();
    }

    ui::imgui_integration::endFrame();
    SDL_GL_SwapWindow(pWindow);
  }
}

} // namespace rigel::ui
