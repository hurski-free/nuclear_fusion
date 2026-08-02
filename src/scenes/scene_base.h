#pragma once

#include "../app_state.h"

#include "simple_ui.h"

#include <memory>

class IScene {
public:
  virtual ~IScene() = default;

  virtual void on_enter(AppState& app) = 0;
  virtual void on_leave() = 0;
  virtual void on_resize(AppState& app, int width, int height) = 0;
  virtual void update(AppState& app, float dt) = 0;

  // Primary interactive scene (used when a scene does not override the
  // helpers below).
  virtual Scene& scene() = 0;

  virtual void handle_messages(UiContext* ctx) { scene().handle_messages(ctx); }
  virtual void update_scene(float dt) { scene().update(dt); }
  virtual void draw(UiContext* ctx) { scene().draw(ctx); }
  virtual void handle_events() { scene().handle_events(); }
};

using ScenePtr = std::unique_ptr<IScene>;

ScenePtr CreateSettingsScene();
ScenePtr CreateGameScene();
