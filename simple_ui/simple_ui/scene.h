#pragma once

#include "simple_ui/component.h"

// One bucket of Scene root components that share the same `layer` value.
struct SceneLayer {
  int layer = 0;
  std::vector<Component*> components;
};

// Owns a list of components and drives the UI frame pipeline.
class SIMPLE_UI_API Scene {
public:
  Scene();
  ~Scene();

  // IMPORTANT: Rebuild `layers` from `components` (ascending layer order).
  // Call after changing the component list or any component's `layer`.
  // Sets components_sorted = true so the renderer can skip per-frame sorting.
  void prepare_scene();

  // Forwards mouse/keyboard input to each component's handle_messages.
  void handle_messages(UiContext* ctx);

  // Forwards dt to each component's update.
  void update(float dt);

  // Collects DrawCommands and submits them to the renderer.
  // Popups from collect_overlay_draw are rendered in a second pass on top of
  // the entire main scene (independent of component layers).
  void draw(UiContext* ctx);

  // Invokes queued component event handlers (call after draw).
  void handle_events();

  // Non-owning pointers; caller keeps component lifetimes valid.
  std::vector<Component*> components;

  // Buckets of components grouped by layer, sorted ascending by layer id.
  // Filled by prepare_scene(); used by draw() when components_sorted is true.
  std::vector<SceneLayer> layers;

  // When true, draw() emits commands via `layers` and the renderer skips sorting.
  bool components_sorted = false;

private:
  // Reused frame list of pointers into component draw buffers.
  std::vector<DrawCommand*> frame_commands_;
};

