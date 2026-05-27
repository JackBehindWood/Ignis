# Project State

## Current Focus
**Project & Settings System — In-Memory Phase COMPLETE.** Ready to move to the next major feature.

## Standing Systems

### Project & Settings (editor-only, in-memory phase complete)
Observer-pattern project dispatch (`IProjectObserver`/`ProjectContext`), mode-aware validate, `EditorAssetManager` as sole engine gateway + project observer, `EditorSettingsManager`, `EngineSettingsManager`, settings-agnostic `Application`, deterministic bootstrap in `Editor::Editor()`.

### Renderer
- Pass-agnostic GRI API with `RenderResourceCache`, `FrameUniformAllocator`, bind helpers, uniform slots
- `RenderGraph` + `RGBuilder` integration — declarative pass graph, barrier inference
- `RenderMesh` pipeline: vertex layout, upload, `MetalGRI` backend
- `RenderShader` / `ShaderCache` / SPIR-V→MSL compilation via SPIRV-Cross
- `MetalCommandContext`, metal-cpp ownership discipline

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
- Improve the build system, add code generation, possibly premake generation and more!
- Scene system overhaul: SceneRenderer with culling/sorting/batching; optional components; Entity ID/Tag component
- Scene serialization (YAML-backed and binary)

## Key Invariants
- Engine TUs: no Metal headers, no `EditorSettings`, no heavy YAML dependency in hot paths
- `IgnisBackend/Metal/`: only TUs here include `<Metal/Metal.hpp>`
- Foundation aliases always; never raw `std::` in TUs
- All new public types: alias in `Foundation/X.h`, added to `igpch.h`
- **Editor invariant**: only `EditorAssetManager.cpp` may include/call `AssetManager` directly
- **Project invariant**: `IProjectObserver` and all project system types are editor-module-only
