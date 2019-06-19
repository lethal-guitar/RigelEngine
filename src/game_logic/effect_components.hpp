/* Copyright (C) 2017, Nikolai Wuttke. All rights reserved.
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

#include "base/array_view.hpp"
#include "base/color.hpp"
#include "base/spatial_types.hpp"
#include "base/warnings.hpp"
#include "data/actor_ids.hpp"
#include "data/sound_ids.hpp"
#include "engine/base_components.hpp"

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <cstdint>
#include <optional>
#include <variant>


namespace rigel { namespace game_logic {

enum class ScoreNumberType : std::uint8_t;


namespace effects {

struct Sound {
  constexpr explicit Sound(const data::SoundId id) noexcept
    : mId(id)
  {
  }

  data::SoundId mId;
};


struct RandomExplosionSound {};


struct Particles {
  Particles(
    const base::Vector& offset,
    const base::Color& color,
    const int velocityScaleX = 0
  )
    : mColor(color)
    , mOffset(offset)
    , mVelocityScaleX(velocityScaleX)
  {
  }

  explicit Particles(
    const base::Vector& offset,
    const int velocityScaleX = 0
  )
    : mOffset(offset)
    , mVelocityScaleX(velocityScaleX)
  {
  }

  std::optional<base::Color> mColor;
  base::Vector mOffset;
  int mVelocityScaleX;
};


struct EffectSprite {
  enum class Movement {
    None = -2,
    FloatUp = -1,
    FlyRight = 0,
    FlyUpperRight = 1,
    FlyUp = 2,
    FlyUpperLeft = 3,
    FlyLeft = 4,
    FlyDown = 5,
    SwirlAround = 6
  };

  base::Vector mOffset;
  data::ActorID mActorId;
  Movement mMovement;
};


struct SpriteCascade {
  data::ActorID mActorId;
};


struct ScoreNumber {
  base::Vector mOffset;
  ScoreNumberType mType;
};


using EffectType = std::variant<
  Sound,
  RandomExplosionSound,
  Particles,
  EffectSprite,
  SpriteCascade,
  ScoreNumber
>;


struct EffectSpec {
  EffectType mEffect;
  int mDelay;
};

}

void triggerEffects(
  entityx::Entity entity, entityx::EntityManager& entityManager);

namespace components {

struct DestructionEffects {
  enum class TriggerCondition {
    OnKilled,
    OnCollision,
    Manual
  };

  using EffectSpecList = base::ArrayView<effects::EffectSpec>;

  explicit DestructionEffects(
    EffectSpecList effectSpecs,
    TriggerCondition condition = TriggerCondition::OnKilled,
    std::optional<engine::components::BoundingBox> cascadePlacementBox =
      std::nullopt
  )
    : mEffectSpecs(effectSpecs)
    , mTriggerCondition(condition)
    , mCascadePlacementBox(cascadePlacementBox)
  {
  }

  EffectSpecList mEffectSpecs;
  TriggerCondition mTriggerCondition;
  std::optional<engine::components::BoundingBox> mCascadePlacementBox;
  int mFramesElapsed = 0;
  bool mActivated = false;
};


struct SpriteCascadeSpawner {
  base::Vector mBasePosition;
  base::Extents mCoveredArea;
  data::ActorID mActorId;
  bool mSpawnedLastFrame = true;
};

}}}
