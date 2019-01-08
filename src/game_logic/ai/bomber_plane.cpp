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

#include "bomber_plane.hpp"

#include "base/match.hpp"
#include "engine/base_components.hpp"
#include "engine/collision_checker.hpp"
#include "engine/entity_tools.hpp"
#include "engine/life_time_components.hpp"
#include "engine/movement.hpp"
#include "engine/physical_components.hpp"
#include "game_logic/effect_components.hpp"
#include "game_logic/entity_factory.hpp"
#include "game_logic/player.hpp"

#include "game_service_provider.hpp"
#include "global_level_events.hpp"


namespace rigel { namespace game_logic { namespace behaviors {

namespace {

using EffectMovement = effects::EffectSprite::Movement;

const effects::EffectSpec BIG_BOMB_DETONATE_IN_AIR_EFFECT_SPEC[] = {
  {effects::RandomExplosionSound{}, 0},
  {effects::EffectSprite{{  0, 0}, 43, EffectMovement::None},    0},
  {effects::EffectSprite{{ -4, 0}, 43, EffectMovement::FlyDown}, 2},
  {effects::EffectSprite{{ +4, 0}, 43, EffectMovement::FlyDown}, 2},
  {effects::EffectSprite{{ -8, 0}, 43, EffectMovement::FlyDown}, 4},
  {effects::EffectSprite{{ +8, 0}, 43, EffectMovement::FlyDown}, 4},
  {effects::EffectSprite{{-12, 0}, 43, EffectMovement::FlyDown}, 6},
  {effects::EffectSprite{{+12, 0}, 43, EffectMovement::FlyDown}, 6},
  {effects::EffectSprite{{-16, 0}, 43, EffectMovement::FlyDown}, 8},
  {effects::EffectSprite{{+16, 0}, 43, EffectMovement::FlyDown}, 8},
};


constexpr auto FLY_AWAY_SPEED_VECTOR = base::Vector{2, 1};
constexpr auto BOMB_OFFSET = base::Vector{2, 0};
constexpr auto BOMB_DROP_OFFSET = base::Vector{2, 1};

}


void BomberPlane::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool isOnScreen,
  entityx::Entity entity
) {
  using namespace engine::components;

  const auto& position = *entity.component<WorldPosition>();

  auto dropBomb = [&]() {
    // There is a little complexity here due to the way the bomb is spawned in
    // the original game. Specifically, it is spawned with the same offset as
    // we do here, but only shows up one frame later, at which point it
    // immediately switches to the 2nd animation frame (slightly angled) before
    // being drawn for the first time. This makes it appear visually lower
    // compared to the 1st animation frame, thus making the bomb appear to be
    // in the correct position. OTOH, if the bomb is rendered with animation
    // frame 0 at the same position, it will appear to be too low though. This
    // leads to a weird visual glitch when the bomb is dropped. In the original
    // game, this isn't noticeable though due to the fact that the bomb only
    // appears one frame later, and a brief moment of invisibility seems to be
    // less noticeable.
    //
    // For RigelEngine, I wanted to fix the one frame of invisibility, but
    // prevent the visual glitch at the same time. I didn't want to mess with
    // the actual positioning of the bomb, as that would in theory alter the
    // gameplay. Thus, I settled on the following:
    //
    // 1) The placeholder sprite (mBombSprite) is shown for one frame longer
    //    after the bomb has been dropped.
    // 2) The bomb is initially made invisibile, to prevent it from overlapping
    //    with the placeholder. It is made visible in BigBomb::update().
    //
    // Together, this results in no visual glitch, but no brief disappearance
    // of the bomb either.
    mBombSprite.assign<AutoDestroy>(AutoDestroy::afterTimeout(1));
    auto bomb = d.mpEntityFactory->createActor(63, position + BOMB_DROP_OFFSET);
    bomb.component<Sprite>()->mShow = false;
  };

  auto flyAway = [&]() {
    // no collision checking while flying away
    *entity.component<WorldPosition>() -= FLY_AWAY_SPEED_VECTOR;
  };


  base::match(mState,
    [&, this](const FlyingIn&) {
      if (!mBombSprite) {
        mBombSprite = d.mpEntityFactory->createSprite(
          63, position + BOMB_OFFSET);
      }

      const auto result =
        engine::moveHorizontally(*d.mpCollisionChecker, entity, -1);

      *mBombSprite.component<WorldPosition>() = position + BOMB_OFFSET;

      const auto reachedWall = result != engine::MovementResult::Completed;
      const auto reachedPlayer =
        position.x <= s.mpPlayer->orientedPosition().x &&
        position.x + 6 >= s.mpPlayer->orientedPosition().x;
      if (reachedWall || reachedPlayer) {
        mState = DroppingBomb{};
      }
    },

    [&, this](DroppingBomb& state) {
      ++state.mFramesElapsed;

      if (state.mFramesElapsed == 9) {
        dropBomb();
      } else if (state.mFramesElapsed == 29) {
        entity.remove<ActivationSettings>();
        entity.assign<AutoDestroy>(
          AutoDestroy::Condition::OnLeavingActiveRegion);

        flyAway();
        mState = FlyingOut{};
      }
    },

    [&](const FlyingOut&) {
      flyAway();
    }
  );
}


void BigBomb::update(
  GlobalDependencies& d,
  GlobalState& s,
  const bool,
  entityx::Entity entity
) {
  using namespace engine::components;

  if (!mStartedFalling) {
    // See comment in BomberPlane::update()
    mStartedFalling = true;
    entity.component<Sprite>()->mShow = true;
  }

  // Normally, the bomb's explosion is triggered in the onCollision callback,
  // but if the bomb spawns in a location where it's already touching the
  // ground (this happens in L3, for example), it would get stuck without
  // exploding if we didn't do this check here.
  const auto& position = *entity.component<WorldPosition>();
  const auto& bbox = *entity.component<BoundingBox>();
  if (d.mpCollisionChecker->isOnSolidGround(position, bbox)) {
    triggerEffects(entity, *d.mpEntityManager);
    d.mpEvents->emit(rigel::events::ScreenFlash{loader::INGAME_PALETTE[15]});
    d.mpServiceProvider->playSound(data::SoundId::BigExplosion);
    entity.destroy();
  }
}


void BigBomb::onKilled(
  GlobalDependencies& d,
  GlobalState&,
  const base::Point<float>&,
  entityx::Entity entity
) {
  // When shot while in the air, a slightly different series of explosions is
  // triggered.
  engine::reassign<components::DestructionEffects>(
    entity, BIG_BOMB_DETONATE_IN_AIR_EFFECT_SPEC);
  triggerEffects(entity, *d.mpEntityManager);

  entity.destroy();
}


void BigBomb::onCollision(
  GlobalDependencies& d,
  GlobalState&,
  const engine::events::CollidedWithWorld&,
  entityx::Entity entity
) {
  d.mpEvents->emit(rigel::events::ScreenFlash{loader::INGAME_PALETTE[15]});
  d.mpServiceProvider->playSound(data::SoundId::BigExplosion);
  entity.destroy();
}

}}}
