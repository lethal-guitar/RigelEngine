#pragma once


namespace rigel { namespace data {

enum class Difficulty {
  Easy = 0,
  Medium = 1,
  Hard = 2
};


struct GameSessionId {
  GameSessionId() = default;
  GameSessionId(const int episode, const int level, const Difficulty difficulty)
    : mEpisode(episode)
    , mLevel(level)
    , mDifficulty(difficulty)
  {
  }

  int mEpisode = 0;
  int mLevel = 0;
  Difficulty mDifficulty = Difficulty::Medium;
};

}}
