# Project State

## Current Focus
**Scene System Redesign — Phases 1–4 complete.** Full IHT pipeline, ComponentRegistry, ScriptRegistry, SceneSerializer, FlyCamera, CameraComponent, SceneRenderer overhaul, and all generated `.gen.h` files are live. Phase 4 cleanup: generator bugs fixed, ScriptComponent serializer special-case wired, all gen files regenerated.

## Standing Systems

### Scene System (Phases 1–4 complete)
- IHT pipeline: `scanner.py` FSM, `generator.py` with codec table, `worker.py` manifest diff, all run as build pre-step
- `ComponentRegistry` + `ScriptRegistry` — descriptor stores with idempotent `register_*` + `validate_unique_ids()`
- Generated: `Components.gen.h`, `CameraComponent.gen.h`, `IDComponent.gen.h`, `NameComponent.gen.h`, `ScriptComponent.gen.h`, `SceneRegistry.gen.h`
- `SceneSerializer` — data-driven, ScriptComponent special-cased for ScriptRegistry delegation
- `SceneCamera` + `CameraComponent` — projection state machine, IHT custom codec path
- `FlyCamera` — engine-side free-look controller (no editor dep)
- `SceneRenderer` — CameraData signature, FrameData UBO binding, flat TransformComponent fields
- `Scene::update` — script dispatch; `get_primary_camera_data()` — optional CameraData query

### Project & Settings (editor-only, in-memory phase complete)
Observer-pattern project dispatch (`IProjectObserver`/`ProjectContext`), mode-aware validate, `EditorAssetManager` as sole engine gateway + project observer, `EditorSettingsManager`, `EngineSettingsManager`, settings-agnostic `Application`, deterministic bootstrap in `Editor::Editor()`.

### Renderer
- Pass-agnostic GRI API with `RenderResourceCache`, `FrameUniformAllocator`, bind helpers, uniform slots
- `RenderGraph` + `RGBuilder` integration — declarative pass graph, barrier inference
- `RenderMesh` pipeline: vertex layout, upload, `MetalGRI` backend
- `RenderShader` / `ShaderCache` / SPIR-V→MSL compilation via SPIRV-Cross
- `MetalCommandContext`, metal-cpp ownership discipline
- **Depth buffer Phase 1 complete**: `GRIViewport::get_depth_texture()` virtual, `MetalViewport` override, `Renderer::get_depth_texture()`/`get_config()`, `RGBuilder::import_viewport_depth()`, `SceneRenderer` now declares depth to RG + PSO formats from `RendererConfig`

### Math & Scene
- Foundation math library (Vec2/3/4, Mat4, Quat)
- ECS scene renderer integration — entity → draw call path

### Asset System V2 *(completed, stable baseline)*
- `AssetManager` v2: binary `.igasset` format, cook pipeline, `AssetHandle<T>`, `AssetType` registry
- `RuntimePathId`: interned 64-bit hash of virtual path string; zero heap per frame after warm-up
- FSEvents watcher (macOS): physical file change → virtual path invalidation via `RuntimePathId`
- `EditorAssetManager` isolation: cook + import tooling stays editor-only, engine sees handle/stream API only

## Completed but Pre-Baseline
- Initial renderer scaffolding (pass 1 – pre-RenderGraph)
- Initial scene system and scene renderer

## Upcoming
- Round-trip validation test: save scene → reload → verify field equality (post-Phase 4 smoke test)
- IHT scan of game-project `src/Scripts/` and generated `register_all_scripts()` (when a game project exists)
- docs/scene.md update to reflect final architecture

## Key Invariants
- Engine TUs: no Metal headers, no `EditorSettings`, no heavy YAML dependency in hot paths
- `IgnisBackend/Metal/`: only TUs here include `<Metal/Metal.hpp>`
- Foundation aliases always; never raw `std::` in TUs
- All new public types: alias in `Foundation/X.h`, added to `igpch.h`
- **Editor invariant**: only `EditorAssetManager.cpp` may include/call `AssetManager` directly
- **Project invariant**: `IProjectObserver` and all project system types are editor-module-only
