/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
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

#include <base/warnings.hpp>
#include <common/game_mode.hpp>
#include <common/game_service_provider.hpp>
#include <data/game_session_data.hpp>
#include <data/sound_ids.hpp>
#include <engine/visual_components.hpp>
#include <game_logic/ientity_factory.hpp>

RIGEL_DISABLE_WARNINGS
#include <entityx/entityx.h>
RIGEL_RESTORE_WARNINGS

#include <optional>


namespace rigel
{

struct MockServiceProvider : public rigel::IGameServiceProvider
{
  void fadeOutScreen() override { }
  void fadeInScreen() override { }

  void playSound(rigel::data::SoundId id) override
  {
    mLastTriggeredSoundId = id;
  }
  void stopSound(rigel::data::SoundId id) override { }
  void stopAllSounds() override { }

  void playMusic(const std::string&) override { }
  void stopMusic() override { }
  void scheduleGameQuit() override { }
  void switchGamePath(const std::filesystem::path&) override { }
  void markCurrentFrameAsWidescreen() override { }
  bool isSharewareVersion() const override { return false; }

  const CommandLineOptions& commandLineOptions() const override
  {
    static auto dummyOptions = CommandLineOptions{};
    return dummyOptions;
  }

  const GameControllerInfo& gameControllerInfo() const override
  {
    static auto dummyInfo = GameControllerInfo{};
    return dummyInfo;
  }

  std::optional<rigel::data::SoundId> mLastTriggeredSoundId;
};


struct FireShotParameters
{
  rigel::game_logic::ProjectileType type;
  rigel::engine::components::WorldPosition position;
  rigel::game_logic::ProjectileDirection direction;
};


struct MockEntityFactory : public rigel::game_logic::IEntityFactory
{
  std::vector<FireShotParameters> mCreateProjectileCalls;

  explicit MockEntityFactory(entityx::EntityManager* pEntityManager)
    : mpEntityManager(pEntityManager)
  {
  }

  entityx::Entity spawnProjectile(
    game_logic::ProjectileType type,
    const engine::components::WorldPosition& pos,
    game_logic::ProjectileDirection direction) override
  {
    mCreateProjectileCalls.push_back(FireShotParameters{type, pos, direction});
    return createMockSpriteEntity();
  }

  void createEntitiesForLevel(
    const data::map::ActorDescriptionList& actors) override
  {
  }

  engine::components::Sprite
    createSpriteForId(const data::ActorID actorID) override
  {
    return {};
  }

  entityx::Entity
    spawnSprite(data::ActorID actorID, bool assignBoundingBox = false) override
  {
    return createMockSpriteEntity();
  }

  entityx::Entity spawnSprite(
    data::ActorID actorID,
    const base::Vec2& position,
    bool assignBoundingBox = false) override
  {
    return createMockSpriteEntity();
  }

  entityx::Entity
    spawnActor(data::ActorID actorID, const base::Vec2& position) override
  {
    return createMockSpriteEntity();
  }

  entityx::EntityManager& entityManager() override { return *mpEntityManager; }

private:
  entityx::Entity createMockSpriteEntity()
  {
    static rigel::engine::SpriteDrawData dummyDrawData;

    auto entity = mpEntityManager->create();
    rigel::engine::components::Sprite sprite{&dummyDrawData, {}};
    entity.assign<rigel::engine::components::Sprite>(sprite);
    return entity;
  }

  entityx::EntityManager* mpEntityManager;
};

} // namespace rigel
