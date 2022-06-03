/* Copyright (C) 2022, Nikolai Wuttke. All rights reserved.
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

#include "custom_quad_batch.hpp"

#include "renderer/renderer.hpp"
#include "renderer/shader.hpp"


namespace rigel::renderer
{

glm::mat4 computeTransformationMatrix(renderer::Renderer* pRenderer)
{
  return renderer::computeTransformationMatrix(
    pRenderer->globalTranslation(),
    pRenderer->globalScale(),
    pRenderer->currentRenderTargetSize());
}

} // namespace rigel::renderer
