/* Copyright (C) 2018, Nikolai Wuttke. All rights reserved.
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

#pragma once

#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "engine/base_components.hpp"
#include "game_logic/input.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS


namespace rigel {
  struct IGameServiceProvider;

  namespace data::map {
    class Map;
  }

  namespace engine {
    class CollisionChecker;
    class ParticleSystem;
    class RandomNumberGenerator;
  }

  namespace game_logic {
    struct IEntityFactory;
    class Player;
  }
}


namespace rigel::game_logic {

struct GlobalDependencies {
  const engine::CollisionChecker* mpCollisionChecker;
  engine::ParticleSystem* mpParticles;
  engine::RandomNumberGenerator* mpRandomGenerator;
  IEntityFactory* mpEntityFactory;
  IGameServiceProvider* mpServiceProvider;
  entityx::EntityManager* mpEntityManager;
  entityx::EventManager* mpEvents;
};


struct PerFrameState {
  PlayerInput mInput;
  base::Extents mCurrentViewPortSize;
  int mNumRadarDishes = 0;
  bool mIsOddFrame = false;
  bool mIsEarthShaking = false;
  bool mCompatibilityModeOn = false;
};


struct GlobalState {
  GlobalState(
    Player* pPlayer,
    const base::Vector* pCameraPosition,
    data::map::Map* pMap,
    const PerFrameState* pPerFrameState
  )
    : mpPlayer(pPlayer)
    , mpCameraPosition(pCameraPosition)
    , mpMap(pMap)
    , mpPerFrameState(pPerFrameState)
  {
  }

  Player* mpPlayer;
  const base::Vector* mpCameraPosition;
  data::map::Map* mpMap;
  const PerFrameState* mpPerFrameState;
};


inline bool isBboxOnScreen(
  const GlobalState& s,
  const engine::components::BoundingBox& bounds
) {
  return engine::isOnScreen(
    bounds,
    *s.mpCameraPosition,
    s.mpPerFrameState->mCurrentViewPortSize);
}

}
