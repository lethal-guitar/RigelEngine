#pragma once

#include <SDL_events.h>
#include <memory>


namespace rigel {

template<typename T>
void startStage(T& stage) {
  stage.start();
}

template<typename T>
void updateStage(T& stage, const engine::TimeDelta dt) {
  stage.updateAndRender(dt);
}


template<typename T>
bool isStageFinished(const T& stage) {
  return stage.isFinished();
}


template<typename T>
bool canStageHandleEvents(const T&) {
  return false;
}

template<typename T>
void forwardEventToStage(const T&, const SDL_Event&) {
  // No-op by default
}


class ModeStage {
public:
  template<typename T>
  explicit ModeStage(T&& item_)
    : mpSelf(std::make_unique<Model<T>>(std::forward<T>(item_)))
  {
  }

  ModeStage(ModeStage&&) noexcept = default;
  ModeStage& operator=(ModeStage&&) noexcept = default;

  friend void startStage(ModeStage& stage) {
    stage.mpSelf->start();
  }

  friend void updateStage(ModeStage& stage, const engine::TimeDelta dt) {
    stage.mpSelf->updateAndRender(dt);
  }

  friend bool isStageFinished(const ModeStage& stage) {
    return stage.mpSelf->isFinished();
  }

  friend bool canStageHandleEvents(const ModeStage& stage) {
    return stage.mpSelf->canHandleEvents();
  }

  friend void forwardEventToStage(ModeStage& stage, const SDL_Event& event) {
    stage.mpSelf->handleEvent(event);
  }

private:
  struct Concept {
    virtual ~Concept() = default;
    virtual void start() = 0;
    virtual void updateAndRender(engine::TimeDelta dt) = 0;
    virtual bool isFinished() const = 0;
    virtual bool canHandleEvents() const = 0;
    virtual void handleEvent(const SDL_Event& event) = 0;
  };

  template<typename T>
  struct Model : public Concept {
    explicit Model(T&& data_)
      : data(std::forward<T>(data_))
    {
    }

    void start() override {
      startStage(data);
    }

    void updateAndRender(engine::TimeDelta dt) override {
      updateStage(data, dt);
    }

    bool isFinished() const override {
      return isStageFinished(data);
    }

    bool canHandleEvents() const override {
      return canStageHandleEvents(data);
    }

    void handleEvent(const SDL_Event& event) override {
      forwardEventToStage(data, event);
    }

    T data;
  };

  std::unique_ptr<Concept> mpSelf;
};

}
